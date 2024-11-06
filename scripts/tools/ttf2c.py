#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import argparse
import freetype
import numpy as np
import math
import binascii

c_head_string="""

#include "font/egui_font_std.h"

// clang-format off

"""

c_head_debug_string="""
/**
 * Font size: {0}
 * Font bit size: {1}
 * TTF file: {2}
 * options: {3}
 */

"""

c_body_string="""


static const egui_font_std_info_t {0}_info = {{
    .font_bit_mode = {1},
    .height = {2},
    .count = {3},
    .char_array = {0}_char_array,
    .pixel_buffer = {0}_pixel_buffer,
}};

extern const egui_font_std_t {0};
EGUI_FONT_SUB_DEFINE_CONST(egui_font_std_t, {0}, &{0}_info);

"""


c_tail_string="""

// clang-format on

"""






def generate_glyphs_data(input_file, text, pixel_size, font_bit_size):
    face = freetype.Face(input_file)
    face.set_pixel_sizes(0, pixel_size)

    glyphs_data = []
    current_index = 0

    width_max = 0
    height_max = 0
    baseline_max = 0

    # foreach all the char in the text, to get the max width and height
    for char in set(text):
        # Get the glyph bitmap
        face.load_char(char)
        bitmap = face.glyph.bitmap
        utf8_encoding = char.encode('utf-8')

        # print("%c: %d, %d" % (char, bitmap.width, bitmap.rows))
        # print(bitmap.buffer)

        if len(bitmap.buffer) == 0:
            continue
        if list(utf8_encoding) == [0xef, 0xbb, 0xbf]:
            continue

        width_max = max(bitmap.width, width_max)
        height_max = max(bitmap.rows, height_max)

        bearing_y = face.glyph.bitmap_top
        
        if bearing_y >= 0 and bearing_y < bitmap.rows:
            baseline = bitmap.rows - bearing_y

            baseline_max = max(baseline, baseline_max)


    # sorted the text, and generate the glyphs data.
    # foreach all the char in the text, to get the glyphs data.
    # print("text: %s" % text)
    for char in sorted(set(text)):
        # print("char: %s" % char)
        face.load_char(char)
        bitmap = face.glyph.bitmap
        utf8_encoding = char.encode('utf-8')

        if utf8_encoding != b'\x20': # space
            if len(bitmap.buffer) == 0:
                continue
            if list(utf8_encoding) == [0xef, 0xbb, 0xbf]:
                continue

            advance_width = math.ceil(face.glyph.advance.x / 64.0)
            width = bitmap.width
            height = bitmap.rows
            bearing_x = face.glyph.bitmap_left
            bearing_y = face.glyph.bitmap_top

            # TODO: handle bearing_x < 0?
            if bearing_x < 0:
                advance_width += -bearing_x
                bearing_x = 0
            # TODO: handle advance_width < width?
            if advance_width < width:
                advance_width = width
            # TODO: handle bearing_y < 0?
            if bearing_y < 0:
                bearing_y = 0
            
            offset_x = bearing_x
            offset_y = pixel_size - (bearing_y + baseline_max)

            # TODO: handle offset_y < 0?
            if offset_y < 0:
                offset_y = 0

            # print("char: %s, bitmap.width: %d, bitmap.height: %d, bitmap_left: %d, bitmap_top: %d, width: %d, height: %d, advance_width: %d, offset_x: %d, offset_y: %d" 
            #       % (char, bitmap.width, bitmap.rows, face.glyph.bitmap_left, face.glyph.bitmap_top, width, height, advance_width, offset_x, offset_y))

            bitmap_array = np.array(bitmap.buffer, dtype=np.uint8).reshape((height, width))
        else:
            advance_width = width_max // 2
            if advance_width == 0:
                advance_width = 1
            width = advance_width
            height = 1
            offset_x = 0
            offset_y = 0

            bitmap_array = np.zeros((height, width), dtype=np.uint8)

        # remove padding, for reducing the size of the bitmap array
        # if width < width_max:
        #    padding = ((0, 0), (0, width_max - width))
        #    bitmap_array = np.pad(bitmap_array, padding, 'constant')

        if font_bit_size == 4:
            def RevBitQuadPerByte(byteArr):
                return ((byteArr & 0x0f) << 4) |  ((byteArr & 0xf0) >> 4)

            # temporary array with 2x reduced width & pad
            (r, c) = np.shape(bitmap_array)
            tmp = np.empty((0, int((c+1)/2)), dtype=np.uint8)

            for cur in bitmap_array:
                bitsArr = np.unpackbits(cur.astype(np.uint8))
                # generate indexes for MSB bit quadruplet every byte
                idx = np.arange(0, np.size(bitsArr), 8)
                idx = np.reshape(np.column_stack(
                    (np.column_stack((idx+0, idx+1)), np.column_stack((idx+2, idx+3)))),
                    (1,-1)),

                # extraction + endianness conversion appended in temp array
                # packbits is taking care of padding
                tmp = np.vstack([tmp, RevBitQuadPerByte(np.packbits(bitsArr[idx]))])

            bitmap_array = tmp

        elif font_bit_size == 2:
            def RevBitPairPerByte(byteArr):
                return ((byteArr & 0x03) << 6) |  ((byteArr & 0xc0) >> 6) | ((byteArr & 0x30) >> 2 ) | ((byteArr & 0x0c) << 2)

            # temporary array with 4x reduced width & pad
            (r, c) = np.shape(bitmap_array)
            tmp = np.empty((0, int((c+3)/4)), dtype=np.uint8)

            for cur in bitmap_array:
                bitsArr = np.unpackbits(cur.astype(np.uint8))

                # generate indexes for MSB bit pair every byte
                idx = np.arange(0, np.size(bitsArr), 8)
                idx = np.reshape(np.column_stack((idx+0, idx+1)), (1,-1))

                # extraction + endianness conversion appended in temp array
                tmp = np.vstack([tmp, RevBitPairPerByte(np.packbits(bitsArr[idx]))])

            bitmap_array = tmp

        elif font_bit_size == 1:
            def RevBitPerByte(byteArr):
                return ((byteArr & 0x01) << 7) | ((byteArr & 0x80) >> 7) | \
                       ((byteArr & 0x02) << 5) | ((byteArr & 0x40) >> 5) | \
                       ((byteArr & 0x04) << 3) | ((byteArr & 0x20) >> 3) | \
                       ((byteArr & 0x08) << 1) | ((byteArr & 0x10) >> 1)

            # temporary array with 8x reduced width & pad
            (r, c) = np.shape(bitmap_array)
            # print(f"char: {char}, r: {r}, c: {c}")
            tmp = np.empty((0, int((c+7)/8)), dtype=np.uint8)
            # print(f"tmp: {len(tmp)}, len: {int((c+7)/8)}")

            for cur in bitmap_array:
                bitsArr = np.unpackbits(cur.astype(np.uint8))
                # print(f"cur: {cur}, bitsArr: {bitsArr}")

                # generate indexes for MSB bit every byte
                idx = np.arange(0, np.size(bitsArr), 8)
                # print(f"idx: {idx}, bitsArr[idx]: {bitsArr[idx]}")

                # extraction + endianness conversion
                tmp = np.vstack([tmp, RevBitPerByte(np.packbits(bitsArr[idx]))])

            bitmap_array = tmp

        char_mask_array = bitmap_array.flatten()

        glyphs_data.append((char, char_mask_array, width, height, advance_width, offset_x, offset_y, utf8_encoding))

    return glyphs_data, width_max, height_max


def utf8_to_c_array(utf8_bytes):
    return '{' + ', '.join([f'0x{byte:02x}' for byte in utf8_bytes]) + '}'

def write_c_code(glyphs_data, text, output_file, name, char_max_width, char_max_height, pixel_size, font_bit_size):

    with open(output_file, "a", encoding="utf-8") as f:
        print("/**", file=f)
        print(" * Total character count: {0} Support Text: ".format(len(text)), file=f)
        print("{0}".format(text), file=f)
        print(" */\n", file=f)

        print("static const uint8_t {0}_pixel_buffer[] = {{\n".format(name), file=f)

        for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
            utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
            utf8_value_str = ("0x%08x" % utf8_value)
            utf8_c_array = utf8_to_c_array(utf8_encoding)
            # print(("// Glyph for character - %s - %s" % (char, utf8_value_str)))
            f.write(("\n    /* Glyph for character \"%s\" %s */\n" % (char, utf8_value_str)))
            hex_str = binascii.hexlify(data).decode()

            for i in range(0, len(hex_str), char_max_width*2):
                line = hex_str[i:i+char_max_width*2]
                spaced_line = ' '.join(f"0x{line[j:j+2]}," for j in range(0, len(line), 2))
                f.write("    ")
                f.write(spaced_line)
                f.write("\n")

        f.write("\n};\n\n")

        print("static const egui_font_std_char_descriptor_t {0}_char_array[] = {{\n".format(name), file=f)

        index = 0
        for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
            utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
            utf8_value_str = ("0x%08x" % utf8_value)
            utf8_c_array = utf8_to_c_array(utf8_encoding)
            # f.write(f"    {{ .idx={index}, .box_w={width}, .box_h={height}, .adv={advance_width}, .off_x={offset_x}, .off_y={offset_y}, .code_len={len(utf8_encoding)}, .code={utf8_value_str} }}, /* \"{char}\" */\n")
            f.write(("    {.idx=%6d, .box_w=%3d, .box_h=%3d, .adv=%3d, .off_x=%3d, .off_y=%3d, .code_len=%d, .code=%s}, /* \"%s\" */\n" % (index, width, height, advance_width, offset_x, offset_y, len(utf8_encoding), utf8_value_str, char)))
            # update index
            raw_data_size = len(data)
            index += raw_data_size


        f.write("};\n")

        print(c_body_string.format( name,
                                    font_bit_size,
                                    pixel_size,
                                    len(glyphs_data)
                                    ), file=f)


def main():
    parser = argparse.ArgumentParser(description='TrueTypeFont to C array converter (v1.0.0)')
    parser.add_argument("-i", "--input",    type=str,   help="Path to the TTF file",                     required=True)
    parser.add_argument("-n", "--name",     type=str,   help="The customized UTF8 font name",            required=True)
    parser.add_argument("-t", "--text",     type=str,   help="Path to the text file",                    required=True)
    parser.add_argument("-p", "--pixelsize",type=int,   help="Font size in pixels, fixed in height",     required=False,    default=32)
    parser.add_argument("-s", "--fontbitsize",type=int, help="Font bit size (1,2,4,8)",                  required=False,    default=1)
    parser.add_argument('-ext', '--external', nargs='?',type = int, default=0, required=False, help="Storage format (0: internal, 1: external)")
    parser.add_argument('-o', '--output', nargs='?',type = str, default="", required=False, help="Specify the output file name (default: input file name with.c extension)")

    if len(sys.argv)==1:
        parser.print_help(sys.stderr)
        sys.exit(1)

    args = parser.parse_args()
    
    inputfile = args.input
    external_type = args.external

    if args.fontbitsize not in [1, 2, 4, 8]:
        print(f'Invalid alpha size={args.fontbitsize}')
        sys.exit(1)

    output_path = ""
    if args.output != "":
        output_path = args.output
    else:
        output_path = os.path.dirname(inputfile)

    name = f"egui_res_font_{args.name.lower()}_{args.pixelsize}_{args.fontbitsize}"
    outfilename = os.path.join(output_path, f"{name}.c")

    # get the options
    options = f"-i {args.input} -n {args.name} -t {args.text} -p {args.pixelsize} -s {args.fontbitsize}"

    with open(outfilename, "w", encoding="utf-8") as outputfile:
        print(c_head_string, file=outputfile)
        print(c_head_debug_string.format(args.pixelsize, args.fontbitsize, args.input, options), file=outputfile)

    # Convert the text file to a list of characters
    with open(args.text, 'r', encoding='utf-8') as f:
        text = f.read()

        glyphs_data, char_max_width, char_max_height = generate_glyphs_data(args.input, text, args.pixelsize, args.fontbitsize)
        write_c_code(glyphs_data, text, outfilename, name, char_max_width, char_max_height, args.pixelsize, args.fontbitsize)

    # write the tail of the c file
    with open(outfilename, "a", encoding="utf-8") as outputfile:
        print(c_tail_string, file=outputfile)


if __name__ == '__main__':
    main()
