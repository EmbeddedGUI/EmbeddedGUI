#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import struct
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

c_head_string_bin="""

#include "font/egui_font_std.h"
#include "app_egui_resource_generate.h"

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
    .font_size = {1},
    .font_bit_mode = {2},
    .height = {3},
    .res_type = {4},
    .count = {5},
    .code_array = {0}_code_array,
    .char_array = (void *){6},
    .pixel_buffer = (void *){7},
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

    ascender = face.size.ascender / 64.0
    descender = face.size.descender / 64.0
    char_max_height = (int)(ascender - descender)

    # print("ascender: %f, descender: %f" % (ascender, descender))

    # foreach all the char in the text, to get the max width and height
    for char in set(text):
        # Get the glyph bitmap
        face.load_char(char)
        bitmap = face.glyph.bitmap
        utf8_encoding = char.encode('utf-8')
        
        advance_width = face.glyph.advance.x / 64.0
        width = bitmap.width
        height = bitmap.rows
        bearing_x = face.glyph.bitmap_left
        bearing_y = face.glyph.bitmap_top

        # height = face.size.height / 64.0

        # print("%c: width: %f, height: %f, bearing_x: %f, bearing_y: %f, advance_width: %f" % (char, width, height, bearing_x, bearing_y, advance_width))
        # print(bitmap.buffer)

        if len(bitmap.buffer) == 0:
            continue
        if list(utf8_encoding) == [0xef, 0xbb, 0xbf]:
            continue

        width_max = max(width, width_max)
        height_max = max(height, height_max)

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

            advance_width = face.glyph.advance.x / 64.0
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
            offset_y = ascender - bearing_y

            # TODO: handle offset_y < 0?
            if offset_y < 0:
                offset_y = 0

            # print("char: %s, width: %f, height: %f, bearing_x: %f, bearing_y: %f, advance_width: %f, offset_x: %f, offset_y: %f" 
            #       % (char, width, height, bearing_x, bearing_y, advance_width, offset_x, offset_y))

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

    return glyphs_data, width_max, char_max_height


def utf8_to_c_array(utf8_bytes):
    return '{' + ', '.join([f'0x{byte:02x}' for byte in utf8_bytes]) + '}'


class ttf2c_tool:
    def __init__(self, input_font_file, name, text_file, pixel_size, font_bit_size, external_type, output_path):
        if output_path == None:
            output_path = os.path.dirname(input_font_file)

        name_raw = f"egui_res_font_{name.lower()}_{pixel_size}_{font_bit_size}"
        font_name = name_raw

        if external_type == 1:
            font_name += "_bin"

        pixel_buffer_bin_name = f"{name_raw}_pixel_buffer"
        char_desc_bin_name = f"{name_raw}_char_desc"

        pixel_buffer_bin_name_res_id = f"EGUI_EXT_RES_ID_{pixel_buffer_bin_name}".upper()
        char_desc_bin_name_res_id = f"EGUI_EXT_RES_ID_{char_desc_bin_name}".upper()

        outfilename = os.path.join(output_path, f"{font_name}.c")
        outfilename_pixel_buffer_bin = os.path.join(output_path, f"{pixel_buffer_bin_name}.bin")
        outfilename_char_desc_bin = os.path.join(output_path, f"{char_desc_bin_name}.bin")

        # just get file name.
        input_font_file_name = os.path.basename(input_font_file)

        # get the options
        options = f"-i {input_font_file_name} -n {name} -p {pixel_size} -s {font_bit_size} -ext {external_type}"

        index = 0
        support_text = ""
        for file in text_file:
            index += 1
            with open(file, 'r', encoding='utf-8') as f:
                support_text += f.read()
                if index < len(text_file):
                    support_text += "\n"
            text_file_name = os.path.basename(file)
            options += f" -t {text_file_name}"

        # save info
        self.input_font_file = input_font_file
        self.name = name
        self.text_file = text_file
        self.pixel_size = pixel_size
        self.font_bit_size = font_bit_size
        self.external_type = external_type
        self.output_path = output_path

        self.font_name = font_name
        self.pixel_buffer_bin_name = pixel_buffer_bin_name
        self.char_desc_bin_name = char_desc_bin_name
        self.pixel_buffer_bin_name_res_id = pixel_buffer_bin_name_res_id
        self.char_desc_bin_name_res_id = char_desc_bin_name_res_id
        self.outfilename = outfilename
        self.outfilename_pixel_buffer_bin = outfilename_pixel_buffer_bin
        self.outfilename_char_desc_bin = outfilename_char_desc_bin
        self.input_font_file_name = input_font_file_name
        self.support_text = support_text
        self.options = options

        self.pixel_buffer_bin_data = None
        self.char_desc_bin_data = None

    def write_c_file(self):
        with open(self.outfilename, "w", encoding="utf-8") as outputfile:
            # insert header
            if self.external_type == 1:
                print(c_head_string_bin, file=outputfile)
            else:
                print(c_head_string, file=outputfile)
            print(c_head_debug_string.format(self.pixel_size, self.font_bit_size, self.input_font_file_name, self.options), file=outputfile)

        # Convert the text file to a list of characters
        glyphs_data, char_max_width, char_max_height = generate_glyphs_data(self.input_font_file, self.support_text, self.pixel_size, self.font_bit_size)
        self.pixel_buffer_bin_data, self.char_desc_bin_data = self.write_c_code(glyphs_data, char_max_width, char_max_height)

        # write the tail of the c file
        with open(self.outfilename, "a", encoding="utf-8") as outputfile:
            print(c_tail_string, file=outputfile)
    
        if self.external_type == 1:
            # print(pixel_buffer_bin_data)
            with open(self.outfilename_pixel_buffer_bin,"wb+") as f:
                f.write(bytearray(self.pixel_buffer_bin_data))
                
            # print(char_desc_bin_data)
            with open(self.outfilename_char_desc_bin,"wb+") as f:
                f.write(bytearray(self.char_desc_bin_data))
    
    def write_c_code(self, glyphs_data, char_max_width, char_max_height):
        index = 0
        bin_pixel_buffer = []
        bin_char_desc = []

        text = self.support_text
        output_file = self.outfilename
        name = self.font_name
        pixel_size = self.pixel_size
        font_bit_size = self.font_bit_size
        external_type = self.external_type

        pixel_buffer_buf_name = '%s_pixel_buffer' % (name)
        char_desc_buf_name = '%s_char_array' % (name)

        res_type = "EGUI_RESOURCE_TYPE_INTERNAL"

        with open(output_file, "a", encoding="utf-8") as f:
            print("/**", file=f)
            print(" * Total character count: {0} Support Text: ".format(len(text)), file=f)
            print("{0}".format(text), file=f)
            print(" */\n", file=f)

            if external_type == 1:
                res_type = "EGUI_RESOURCE_TYPE_EXTERNAL"
                # pixel buffer
                pixel_buffer_buf_name = self.pixel_buffer_bin_name_res_id
                for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
                    utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
                    utf8_value_str = ("0x%08x" % utf8_value)
                    utf8_c_array = utf8_to_c_array(utf8_encoding)
                    bin_pixel_buffer += data.tolist()
                    # print(data)

                # char buffer
                char_desc_buf_name = self.char_desc_bin_name_res_id
                for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
                    utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
                    utf8_value_str = ("0x%08x" % utf8_value)
                    utf8_c_array = utf8_to_c_array(utf8_encoding)
                    raw_data_size = len(data)

                    # typedef struct
                    # {
                    #     uint32_t idx;     // the buffer size is 32bit
                    #     uint16_t size;    // the buffer size
                    #     uint8_t box_w;    // we never use big font, so we don't need to support big font
                    #     uint8_t box_h;    // we never use big font, so we don't need to support big font
                    #     uint8_t adv;      // the distance from the origin to the next character
                    #     int8_t off_x;     // the x offset of the character
                    #     int8_t off_y;     // the y offset of the character
                    # } egui_font_std_char_descriptor_t;

                    char_desc_data = bytearray()
                    char_desc_data.extend(struct.pack('I', index))
                    char_desc_data.extend(struct.pack('H', raw_data_size))
                    char_desc_data.extend(struct.pack('B', width))
                    char_desc_data.extend(struct.pack('B', height))
                    char_desc_data.extend(struct.pack('B', int(advance_width)))
                    char_desc_data.extend(struct.pack('b', int(offset_x)))
                    char_desc_data.extend(struct.pack('b', int(offset_y)))
                    char_desc_data.extend(struct.pack('b', 0)) # align to 4 bytes

                    bin_char_desc += list(char_desc_data)

                    # update index
                    index += raw_data_size
            else:
                # pixel buffer
                print("static const uint8_t {0}[] = {{\n".format(pixel_buffer_buf_name), file=f)

                for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
                    utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
                    utf8_value_str = ("0x%08x" % utf8_value)
                    utf8_c_array = utf8_to_c_array(utf8_encoding)
                    bin_pixel_buffer += data.tolist()
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

                # char buffer
                print("static const egui_font_std_char_descriptor_t {0}[] = {{\n".format(char_desc_buf_name), file=f)

                for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
                    utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
                    utf8_value_str = ("0x%08x" % utf8_value)
                    utf8_c_array = utf8_to_c_array(utf8_encoding)
                    raw_data_size = len(data)
                    f.write(("    {.idx=%6d, .size=%6d, .box_w=%3d, .box_h=%3d, .adv=%3d, .off_x=%3d, .off_y=%3d}, /* \"%s\" %s */\n" % (index, raw_data_size, width, height, advance_width, offset_x, offset_y, char, utf8_value_str)))
                    
                    # typedef struct
                    # {
                    #     uint32_t idx;     // the buffer size is 32bit
                    #     uint16_t size;    // the buffer size
                    #     uint8_t box_w;    // we never use big font, so we don't need to support big font
                    #     uint8_t box_h;    // we never use big font, so we don't need to support big font
                    #     uint8_t adv;      // the distance from the origin to the next character
                    #     int8_t off_x;     // the x offset of the character
                    #     int8_t off_y;     // the y offset of the character
                    # } egui_font_std_char_descriptor_t;

                    char_desc_data = bytearray()
                    char_desc_data.extend(struct.pack('I', index))
                    char_desc_data.extend(struct.pack('H', raw_data_size))
                    char_desc_data.extend(struct.pack('B', width))
                    char_desc_data.extend(struct.pack('B', height))
                    char_desc_data.extend(struct.pack('B', int(advance_width)))
                    char_desc_data.extend(struct.pack('b', int(offset_x)))
                    char_desc_data.extend(struct.pack('b', int(offset_y)))
                    char_desc_data.extend(struct.pack('b', 0)) # align to 4 bytes

                    bin_char_desc += list(char_desc_data)
                    
                    # update index
                    index += raw_data_size


                f.write("};\n")

            
            # code buffer
            print("static const egui_font_std_code_descriptor_t {0}_code_array[] = {{\n".format(name), file=f)

            for char, data, width, height, advance_width, offset_x, offset_y, utf8_encoding in glyphs_data:
                utf8_value = int.from_bytes(utf8_encoding, byteorder='big')
                utf8_value_str = ("0x%08x" % utf8_value)
                utf8_c_array = utf8_to_c_array(utf8_encoding)
                raw_data_size = len(data)
                f.write(("    {.code=%s}, /* \"%s\" */\n" % (utf8_value_str, char)))
                # update index
                index += raw_data_size


            f.write("};\n")

            print(c_body_string.format( name,
                                        pixel_size,
                                        font_bit_size,
                                        char_max_height,
                                        res_type,
                                        len(glyphs_data),
                                        char_desc_buf_name,
                                        pixel_buffer_buf_name,
                                        ), file=f)
            
        return bin_pixel_buffer, bin_char_desc

def main():
    parser = argparse.ArgumentParser(description='TrueTypeFont to C array converter (v1.0.0)')
    parser.add_argument("-i", "--input",    type=str,   help="Path to the TTF file",                     required=True)
    parser.add_argument("-n", "--name",     type=str,   help="The customized UTF8 font name",            required=True)
    parser.add_argument("-t", "--text",     action='append', type=str,   help="Path to the text file",                    required=True)
    parser.add_argument("-p", "--pixelsize",type=int,   help="Font size in pixels, fixed in height",     required=False,    default=32)
    parser.add_argument("-s", "--fontbitsize",type=int, help="Font bit size (1,2,4,8)",                  required=False,    default=1)
    parser.add_argument('-ext', '--external', nargs='?',type = int, default=0, required=False, help="Storage format (0: internal, 1: external)")
    parser.add_argument('-o', '--output', nargs='?',type = str, default="", required=False, help="Specify the output file name (default: input file name with.c extension)")

    if len(sys.argv)==1:
        parser.print_help(sys.stderr)
        sys.exit(1)

    args = parser.parse_args()
    
    tool = ttf2c_tool(args.input, args.name, args.text, args.pixelsize, args.fontbitsize, args.external, args.output)

    tool.write_c_file()


if __name__ == '__main__':
    main()
