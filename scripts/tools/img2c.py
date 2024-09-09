#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
from PIL import Image
import numpy as np
import time;
import argparse
import os


c_head_string="""

#include "image/egui_image_std.h"

// clang-format off

"""

c_head_debug_string="""
/**
 * Image File : {0}
 * Format : {1}
 * Alpha Type : {2}
 * Re-sized : {3}
 * Rotation : {4}
 * options: {5}
 */

"""


c_body_string="""


static const egui_image_std_info_t {0}_info = {{
    .data_buf = (void *){0}_data_buf,
    .alpha_buf = (void *){1},
    .data_type = {2},
    .alpha_type = {3},
    .width = {4},
    .height = {5},
}};

extern const egui_image_std_t {0};
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_std_t, {0}, &{0}_info);

"""

c_tail_string="""

// clang-format on

"""

def main(argv):

    parser = argparse.ArgumentParser(description='image to C array converter (v1.2.2)')

    parser.add_argument('-i', "--input", nargs='?', type = str,  required=True, help="Input file (png, bmp, etc..)")
    parser.add_argument('-n', "--name", nargs='?',type = str, required=True, help="The customized UTF8 image name")
    parser.add_argument('-f', '--format', nargs='?',type = str, default="rgb565", required=False, help="RGB Format (rgb32, rgb565, gray8)")
    parser.add_argument('-a', '--alpha', nargs='?',type = int, default=8, required=False, help="Alpha Format (0, 1, 2, 4, 8)")
    parser.add_argument('-d', '--dim', nargs=2,type = int, required=False, help="Resize the image with the given width and height")
    parser.add_argument('-r', '--rot', nargs='?',type = float, default=0.0, required=False, help="Rotate the image with the given angle in degrees")

    args = parser.parse_args()

    inputfile = args.input

    if args.format != 'rgb32' and \
        args.format != 'rgb565' and \
        args.format != 'gray8':
        parser.print_help()
        exit(1)

    try:
        image = Image.open(inputfile)
    except FileNotFoundError:
        print("Cannot open image file %s" % (inputfile))
        sys.exit(2)

    # get the options
    options = f"-i {args.input} -n {args.name} -f {args.format} -a {args.alpha}"

    # rotation
    if args.rot != 0.0:
        image = image.rotate(args.rot)
        options += f" -r {args.rot}"


    # resizing
    resized = False
    if args.dim != None:
        image = image.resize((args.dim[0], args.dim[1]))
        resized = True
        options += f" -d {args.dim[0]} {args.dim[1]}"

    name = f"egui_res_image_{args.name.lower()}_{args.format}_{args.alpha}"

    outfilename = os.path.join(os.path.dirname(inputfile), f"{name}.c")

    mode = image.mode

    # Modes supported by Pillow

    # 1 (1-bit pixels, black and white, stored with one pixel per byte), the value is in 0-1.
    # L (8-bit pixels, black and white), the value is in 0-255.
    # P (8-bit pixels, mapped to any other mode using a color palette), the value is in 0-255.
    # RGB (3×8-bit pixels, true color), the value is in [0-255, 0-255, 0-255].
    # RGBA (4×8-bit pixels, true color with transparency mask), the value is in [0-255, 0-255, 0-255, 0-255]
    # CMYK (4×8-bit pixels, color separation)
    # YCbCr (3×8-bit pixels, color video format)
    # LAB (3×8-bit pixels, the L*a*b color space)
    # HSV (3×8-bit pixels, Hue, Saturation, Value color space)
    # I (32-bit signed integer pixels)
    # F (32-bit floating point pixels)
    # LA (L with alpha)
    # PA (P with alpha)
    # RGBX (true color with padding)
    # RGBa (true color with premultiplied alpha)
    # La (L with premultiplied alpha)
    # I;16 (16-bit unsigned integer pixels)
    # I;16L (16-bit little endian unsigned integer pixels)
    # I;16B (16-bit big endian unsigned integer pixels)
    # I;16N (16-bit native endian unsigned integer pixels)
    # BGR;15 (15-bit reversed true colour)
    # BGR;16 (16-bit reversed true colour)
    # BGR;24 (24-bit reversed true colour)
    # BGR;32 (32-bit reversed true colour)

    # handle {P, LA, RGB, RGBA} for now
    if mode == 'P' or mode == 'LA':
        image = image.convert('RGBA')
        mode = 'RGBA'
    if mode == 'L':
        image = image.convert('RGB')
        mode = 'RGB'
    (row, col) = image.size
    data = np.asarray(image)

    # C Array format width
    WIDTH_ALPHA = 16
    WIDTH_GRAY8 = 32
    WIDTH_RGB565 = 16
    WIDTH_RGB32 = 16

    with open(outfilename,"w+") as o:
        # insert header
        print(c_head_string, file=o)
        print(c_head_debug_string.format(args.input, args.format, args.alpha, resized, args.rot, options), file=o)

        alpha_buf = "NULL"

        alpha_type = "EGUI_IMAGE_ALPHA_TYPE_8"
        data_type = "EGUI_IMAGE_DATA_TYPE_RGB32"
        if mode == "RGBA":
            alpha_buf = '%s_alpha_buf' % (name)
            if args.format == 'rgb32':
                # empty alpha.
                alpha_buf = "NULL"
            else:
                # 8-bit Alpha channel
                if args.alpha == 8:
                    alpha_type = "EGUI_IMAGE_ALPHA_TYPE_8"
                    # alpha channel array available
                    print('static const uint8_t %s_alpha_buf[%d*%d] = {' % (name, row, col),file=o)
                    cnt = 0
                    for eachRow in data:
                        lineWidth=0
                        print("/* -%d- */" % (cnt), file=o)
                        for eachPix in eachRow:
                            alpha = eachPix[3]
                            if lineWidth % WIDTH_ALPHA == (WIDTH_ALPHA - 1):
                                print("0x%02x," %(alpha) ,file=o)
                            else:
                                print("0x%02x" %(alpha), end =", ",file=o)
                            lineWidth+=1
                        cnt+=1
                        print('',file=o)
                    print('};', file=o)
                # 4-bit Alpha channel
                elif args.alpha == 4:
                    alpha_type = "EGUI_IMAGE_ALPHA_TYPE_4"
                    def RevBitQuadPerByte(byteArr):
                        return ((byteArr & 0x0f) << 4) |  ((byteArr & 0xf0) >> 4)

                    print('static const uint8_t %s_alpha_buf[%d*%d] = {' % (name, (row+1)/2, col),file=o)
                    cnt = 0
                    alpha = data[...,3].astype(np.uint8)
                    for eachRow in alpha:
                        lineWidth=0
                        print("/* -%d- */" % (cnt), file=o)

                        bitsArr = np.unpackbits(eachRow.astype(np.uint8))

                        # generate indexes for MSB bit quadruplet every byte
                        idx = np.arange(0, np.size(bitsArr), 8)
                        idx=np.reshape(np.column_stack(
                                (np.column_stack((idx+0, idx+1)), np.column_stack((idx+2, idx+3)))),
                                (1,-1)),

                        # extraction + endianness conversion
                        packedBytes = RevBitQuadPerByte(np.packbits(bitsArr[idx]))

                        for elt in packedBytes:
                            if lineWidth % WIDTH_ALPHA == (WIDTH_ALPHA - 1):
                                print("0x%02x," %(elt) ,file=o)
                            else:
                                print("0x%02x" %(elt), end =", ",file=o)
                            lineWidth+=1
                        cnt+=1
                        print('',file=o)
                    print('};', file=o)
                # 2-bit Alpha channel
                elif args.alpha == 2:
                    alpha_type = "EGUI_IMAGE_ALPHA_TYPE_2"
                    def RevBitPairPerByte(byteArr):
                        return ((byteArr & 0x03) << 6) |  ((byteArr & 0xc0) >> 6) | ((byteArr & 0x30) >> 2 ) | ((byteArr & 0x0c) << 2)

                    print('static const uint8_t %s_alpha_buf[%d*%d] = {' % (name, (row+3)/4, col),file=o)
                    cnt = 0
                    alpha = data[...,3].astype(np.uint8)
                    for eachRow in alpha:
                        lineWidth=0
                        print("/* -%d- */" % (cnt), file=o)

                        bitsArr = np.unpackbits(eachRow.astype(np.uint8))

                        # generate indexes for MSB bit pair every byte
                        idx = np.arange(0, np.size(bitsArr), 8)
                        idx=np.reshape(np.column_stack((idx+0, idx+1)), (1,-1))

                        # extraction + endianness conversion
                        packedBytes = RevBitPairPerByte(np.packbits(bitsArr[idx]))

                        for elt in packedBytes:
                            if lineWidth % WIDTH_ALPHA == (WIDTH_ALPHA-1):
                                print("0x%02x," %(elt) ,file=o)
                            else:
                                print("0x%02x" %(elt), end =", ",file=o)
                            lineWidth+=1
                        cnt+=1
                        print('',file=o)
                    print('};', file=o)
                # 1-bit Alpha channel
                elif args.alpha == 1:
                    alpha_type = "EGUI_IMAGE_ALPHA_TYPE_1"
                    def RevBitPairPerByte(byteArr):
                        return ((byteArr & 0x01) << 7) | ((byteArr & 0x80) >> 7) | \
                            ((byteArr & 0x02) << 5) | ((byteArr & 0x40) >> 5) | \
                            ((byteArr & 0x04) << 3) | ((byteArr & 0x20) >> 3) | \
                            ((byteArr & 0x08) << 1) | ((byteArr & 0x10) >> 1)

                    print('static const uint8_t %s_alpha_buf[%d*%d] = {' % (name, (row+7)/8, col),file=o)
                    cnt = 0
                    alpha = data[...,3].astype(np.uint8)
                    for eachRow in alpha:
                        lineWidth=0
                        print("/* -%d- */" % (cnt), file=o)

                        bitsArr = np.unpackbits(eachRow.astype(np.uint8))

                        # generate indexes for MSB bit pair every byte
                        idx = np.arange(0, np.size(bitsArr), 8)

                        # extraction + endianness conversion
                        packedBytes = RevBitPairPerByte(np.packbits(bitsArr[idx]))

                        for elt in packedBytes:
                            if lineWidth % WIDTH_ALPHA == (WIDTH_ALPHA-1):
                                print("0x%02x," %(elt) ,file=o)
                            else:
                                print("0x%02x" %(elt), end =", ",file=o)
                            lineWidth+=1
                        cnt+=1
                        print('',file=o)
                    print('};', file=o)
                else:
                    alpha_buf = "NULL"



        # Gray8 channel array
        if args.format == 'gray8':
            data_type = "EGUI_IMAGE_DATA_TYPE_GRAY8"

            R = (data[...,0]).astype(np.uint16)
            G = (data[...,1]).astype(np.uint16)
            B = (data[...,2]).astype(np.uint16)
            # merge
            RGB = np.rint((R + G + B)/3).astype(np.uint8)

            print('',file=o)
            print('static const uint8_t %s_data_buf[%d*%d] = {' % (name, row, col), file=o)
            cnt = 0
            for eachRow in RGB:
                lineWidth=0
                print("/* -%d- */" % (cnt), file=o)
                for eachPix in eachRow:
                    if lineWidth % WIDTH_GRAY8 == (WIDTH_GRAY8 - 1):
                        print("0x%02x," %(eachPix) ,file=o)
                    else:
                        print("0x%02x" %(eachPix), end =", ", file=o)
                    lineWidth+=1
                print('',file=o)
                cnt+=1
            print('};', file=o)

        # RGB565 channel array
        elif args.format == 'rgb565':
            data_type = "EGUI_IMAGE_DATA_TYPE_RGB565"
            R = (data[...,0]>>3).astype(np.uint16) << 11
            G = (data[...,1]>>2).astype(np.uint16) << 5
            B = (data[...,2]>>3).astype(np.uint16)
            # merge
            RGB = R | G | B

            print('',file=o)
            print('static const uint16_t %s_data_buf[%d*%d] = {' % (name, row, col), file=o)
            cnt = 0
            for eachRow in RGB:
                lineWidth=0
                print("/* -%d- */" % (cnt), file=o)
                for eachPix in eachRow:
                    if lineWidth % WIDTH_RGB565 == (WIDTH_RGB565 - 1):
                        print("0x%04x," %(eachPix) ,file=o)
                    else:
                        print("0x%04x" %(eachPix), end =", ", file=o)
                    lineWidth+=1
                print('',file=o)
                cnt+=1
            print('};', file=o)

        elif args.format == 'rgb32':
            data_type = "EGUI_IMAGE_DATA_TYPE_RGB32"
            R = data[...,0].astype(np.uint32) << 16
            G = data[...,1].astype(np.uint32) << 8
            B = data[...,2].astype(np.uint32)
            if mode == "RGBA":
                A = data[...,3].astype(np.uint32) << 24
            else:
                # alpha chanel forced to 0xFF
                A = 0xff << 24
            # merge
            RGB = R | G | B | A

            print('',file=o)
            print('static const uint32_t %s_data_buf[%d*%d] = {' % (name, row, col), file=o)

            cnt = 0
            for eachRow in RGB:
                lineWidth=0
                print("/* -%d- */" % (cnt), file=o)
                for eachPix in eachRow:
                    if lineWidth % WIDTH_RGB32 == (WIDTH_RGB32 - 1):
                        print("0x%08x," %(eachPix) ,file=o)
                    else:
                        print("0x%08x" %(eachPix), end =", ", file=o)
                    lineWidth+=1
                print('',file=o)
                cnt+=1
            print('};', file=o)
        
        # insert tail
        print(c_body_string.format( name,
                                    alpha_buf,
                                    data_type,
                                    alpha_type,
                                    row,
                                    col
                                    ), file=o)


        print(c_tail_string, file=o)

if __name__ == '__main__':
    main(sys.argv[1:])
