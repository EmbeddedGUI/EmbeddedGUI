import sys
from PIL import Image
import numpy as np
import time;
import argparse
import os


c_head_string="""

#include "core/egui_canvas.h"

// clang-format off

"""

c_head_debug_string="""
/**
 * Simple support radius range : 0-{0}
 * options: {1}
 */

"""

c_head_debug_string_spec="""
/**
 * options: {0}
 */

"""


c_tail_string="""

// clang-format on

"""





















def print_data_array(data):
    for row in data:
        index = 0
        print(("     "), end=' ')
        for item in row:
            print(("[%02d]" % index), end=' ')
            index+=1
        # next line
        print("")
        break
    index_row = 0
    for row in data:
        print(("[%03d]" % index_row), end=' ')
        index_row += 1
        for item in row:
            print((" %03d" % item), end=' ')
        # next line
        print("")


def draw_point(x, y, color, data):
    data[y][x] = color

# data_selected = []
def draw_fillrect(x, y, w, h, color, data):
    for sel_y in range(y, y+h):
        for sel_x in range(x, x+w):
            draw_point(sel_x, sel_y, color, data)


def draw_vline(x, y, l, color, data):
    draw_fillrect(x, y, 1, l, color, data)

def draw_hline(x, y, l, color, data):
    draw_fillrect(x, y, l, 1, color, data)



def restore_circle_by_criticle_info(width, height, radius, data_info_array, data_value_array):
    # check if we can restore the original data
    data_restored = np.zeros((height, width))

    center_x = radius
    center_y = radius
    
    # draw the left-top part
    row_index = 0
    for info in data_info_array:
        start_offset, valid_count, data_value_offset = info
        for i in range(valid_count):
            col_index = row_index + start_offset + i            
            sel_x = radius - col_index
            sel_y = radius - row_index
            draw_point(center_x + (-sel_x), center_y + (-sel_y), data_value_array[data_value_offset + i], data_restored)
            # skip the diagonal write twice
            if sel_x == sel_y:
                continue
            draw_point(center_x + (-sel_y), center_y + (-sel_x), data_value_array[data_value_offset + i], data_restored)

        # write the reserve value to reserve the space, here can use write line or write column
        offset = row_index + start_offset + valid_count
        sel_x = radius - offset
        sel_y = radius - row_index
        draw_hline(center_x + (-sel_x), center_y + (-sel_y), radius - offset, 0xFF, data_restored)
        draw_vline(center_x + (-sel_y), center_y + (-sel_x), radius - offset, 0xFF, data_restored)
        
        row_index += 1
    
    # write reserve value with rect
    sel_x = radius - row_index
    sel_y = radius - row_index
    draw_fillrect(center_x + (-sel_x), center_y + (-sel_y), radius - row_index, radius - row_index, 0xFF, data_restored)











    # draw the left-bottom part
    row_index = 0
    for info in data_info_array:
        start_offset, valid_count, data_value_offset = info
        for i in range(valid_count):
            col_index = row_index + start_offset + i            
            sel_x = radius - col_index
            sel_y = radius - row_index
            draw_point(center_x + (-sel_x), center_y + (sel_y), data_value_array[data_value_offset + i], data_restored)
            # skip the diagonal write twice
            if sel_x == sel_y:
                continue
            draw_point(center_x + (-sel_y), center_y + (sel_x), data_value_array[data_value_offset + i], data_restored)

        # write the reserve value to reserve the space, here can use write line or write column
        offset = row_index + start_offset + valid_count
        sel_x = radius - offset
        sel_y = radius - row_index
        draw_hline(center_x + (-sel_x), center_y + (sel_y), radius - offset, 0xFF, data_restored)
        
        # change direction
        sel_x = 1
        draw_vline(center_x + (-sel_y), center_y + (sel_x), radius - offset, 0xFF, data_restored)
        
        row_index += 1
    
    # write reserve value with rect
    sel_x = radius - row_index
    sel_y = 1
    draw_fillrect(center_x + (-sel_x), center_y + (sel_y), radius - row_index, radius - row_index, 0xFF, data_restored)
















    # draw the right-top part
    row_index = 0
    for info in data_info_array:
        start_offset, valid_count, data_value_offset = info
        for i in range(valid_count):
            col_index = row_index + start_offset + i            
            sel_x = radius - col_index
            sel_y = radius - row_index
            draw_point(center_x + (sel_x), center_y + (-sel_y), data_value_array[data_value_offset + i], data_restored)
            # skip the diagonal write twice
            if sel_x == sel_y:
                continue
            draw_point(center_x + (sel_y), center_y + (-sel_x), data_value_array[data_value_offset + i], data_restored)

        # write the reserve value to reserve the space, here can use write line or write column
        offset = row_index + start_offset + valid_count
        sel_x = 1
        sel_y = radius - row_index
        draw_hline(center_x + (sel_x), center_y + (-sel_y), radius - offset, 0xFF, data_restored)
        
        # change direction
        sel_x = radius - offset
        draw_vline(center_x + (sel_y), center_y + (-sel_x), radius - offset, 0xFF, data_restored)
        
        row_index += 1
    
    # write reserve value with rect
    sel_x = 1
    sel_y = radius - row_index
    draw_fillrect(center_x + (sel_x), center_y + (-sel_y), radius - row_index, radius - row_index, 0xFF, data_restored)














    # draw the right-bottom part
    row_index = 0
    for info in data_info_array:
        start_offset, valid_count, data_value_offset = info
        for i in range(valid_count):
            col_index = row_index + start_offset + i            
            sel_x = radius - col_index
            sel_y = radius - row_index
            draw_point(center_x + (sel_x), center_y + (sel_y), data_value_array[data_value_offset + i], data_restored)
            # skip the diagonal write twice
            if sel_x == sel_y:
                continue
            draw_point(center_x + (sel_y), center_y + (sel_x), data_value_array[data_value_offset + i], data_restored)

        # write the reserve value to reserve the space, here can use write line or write column
        offset = row_index + start_offset + valid_count
        sel_x = 1
        sel_y = radius - row_index
        draw_hline(center_x + (sel_x), center_y + (sel_y), radius - offset, 0xFF, data_restored)
        draw_vline(center_x + (sel_y), center_y + (sel_x), radius - offset, 0xFF, data_restored)
        
        row_index += 1
    
    # write reserve value with rect
    sel_x = 1
    sel_y = 1
    draw_fillrect(center_x + (sel_x), center_y + (sel_y), radius - row_index, radius - row_index, 0xFF, data_restored)






    # draw the center line
    draw_hline(center_x - radius, center_y,          radius, 0xFF, data_restored)
    draw_hline(center_x + 1,      center_y,          radius, 0xFF, data_restored)
    draw_vline(center_x,          center_y - radius, (radius << 1) + 1, 0xFF, data_restored)
    # draw_vline(center_x,          center_y - radius, radius, 0xFF, data_restored)
    # draw_vline(center_x,          center_y + 1,      radius, 0xFF, data_restored)
    # draw_point(center_x,          center_y, 0xFF, data_restored)

    return data_restored




def draw_filled_circle(radius):
    print("Drawing filled circle with radius", radius)
    width = radius * 2 + 1
    height = radius * 2 + 1

    # create a 20x20 array of zeros
    data = np.zeros((height, width))

    # print_data_array(data)

    # set the center of the circle to be at (10, 10)
    # center = (radius + 5, radius + 5)
    center = (radius, radius)

    # loop through all the pixels in the image
    for y in range(height):
        for x in range(width):
            distance_x = x - center[0]
            distance_y = y - center[1]
            # calculate the distance from the center of the circle
            distance = (distance_x * distance_x + distance_y * distance_y)**0.5
            # if the distance is less than or equal to the radius, set the pixel to 1
            if distance <= radius:
                draw_point(x, y, 255, data)
            elif distance <= radius + 1:
                draw_point(x, y, 255 - int(255 * (distance - radius) / 1), data)

    # print_data_array(data)

    # save the image
    img = Image.fromarray(data.astype('uint8'))
    img.save('circle.png')



def get_filled_circle_info(radius):
    print("Get filled circle info with radius", radius)
    width = radius * 2 + 1
    height = radius * 2 + 1

    # create a 20x20 array of zeros
    data = np.zeros((width, height))

    # set the center of the circle to be at (10, 10)
    # center = (radius + 5, radius + 5)
    center = (radius, radius)

    # loop through all the pixels in the image
    for y in range(height):
        for x in range(width):
            distance_x = x - center[0]
            distance_y = y - center[1]
            # calculate the distance from the center of the circle
            distance = (distance_x * distance_x + distance_y * distance_y)**0.5
            # if the distance is less than or equal to the radius, set the pixel to 1
            if distance <= radius:
                draw_point(x, y, 255, data)
            elif distance <= radius + 1:
                draw_point(x, y, 255 - int(255 * (distance - radius) / 1), data)

    # print_data_array(data)

    # save the image
    # img = Image.fromarray(data.astype('uint8'))
    # img.save('circle.png')

    # follow the center circle algorithm.
    # start caclulating the info of the circle
    data_info_array = []
    data_value_array = []
    data_value_offset = 0
    for x in range(radius):
        start_offset = 0
        valid_count = 0
        for y in range(x, radius):
            if data[y][x] == 0:
                start_offset += 1
            elif data[y][x] == 255:
                break
            else:
                valid_count += 1
                data_value_array.append((int)(data[y][x]))
        # save info
        if valid_count == 0:
            break
        data_info_array.append((start_offset, valid_count, data_value_offset))
        data_value_offset += valid_count

    # print("data_info_array, len: %d, content: %s" % (len(data_info_array), data_info_array))
    # print("data_value_array, len: %d, content: %s" % (len(data_value_array), data_value_array))

    data_restored = restore_circle_by_criticle_info(width, height, radius, data_info_array, data_value_array)

    # print_data_array(data_restored)

    # check if we can restore the original data
    if not (data_restored == data).all():
        print("Restored data is different from the original data")
        return None

    return (data_info_array, data_value_array)




def draw_circle(radius):
    width = radius * 2 + 1
    height = radius * 2 + 1

    # create a 20x20 array of zeros
    data = np.zeros((width, height))

    # print_data_array(data)

    # set the center of the circle to be at (10, 10)
    # center = (radius + 5, radius + 5)
    center = (radius, radius)

    # loop through all the pixels in the image
    for x in range(width):
        for y in range(height):
            distance_x = x - center[0]
            distance_y = y - center[1]
            # calculate the distance from the center of the circle
            distance = (distance_x * distance_x + distance_y * distance_y)**0.5

            diff = abs(distance - radius)
            # if the distance is less than or equal to the radius, set the pixel to 1
            if diff <= 1:
                data[x][y] = 255 - int(255 * (diff) / 1)

    print_data_array(data)

    # save the image
    img = Image.fromarray(data.astype('uint8'))
    img.save('circle.png')




# draw_filled_circle(1)
# draw_filled_circle(2)
# draw_filled_circle(3)
# draw_filled_circle(4)
# draw_filled_circle(5)
# draw_filled_circle(6)
# draw_filled_circle(7)
# draw_filled_circle(8)
# draw_filled_circle(9)

# draw_circle(6)
# get_filled_circle_info(9)























def main(argv):
    parser = argparse.ArgumentParser(description='circle to C array converter (v1.0.0)')
    
    parser.add_argument('-r', '--range', nargs='?',type = int, default=300, required=False, help="Default radius range is 0-1000")
    parser.add_argument('-s', "--spec", nargs='+',type = int, required=False, help="Spec the specific radius, like 100 200 300 400 500")

    args = parser.parse_args()

    if args.spec:
        # get the options
        args.spec = sorted(args.spec) # sort the spec list
        options = "-s " + " ".join(map(str, args.spec))
        
        name = f"egui_res_circle_info_spec"
        outfilename = f"{name}.c"
        
        with open(outfilename,"w") as f:
            # insert header
            print(c_head_string, file=f)
            print(c_head_debug_string_spec.format(options), file=f)

            data_info_array_saved = []
            # print the circle info
            for i in args.spec:
                data_info_array, data_value_array = get_filled_circle_info(i)
                data_info_array_saved.append(data_info_array)

                
                print('static const egui_circle_item_t %s_item_arr_sub_%d[%d] = {' % (name, i, len(data_info_array)), file=f)
                row_index = 0
                for info in data_info_array:
                    start_offset, valid_count, data_value_offset = info
                    start_offset += row_index
                    if start_offset > (1 << 10) - 1:
                        raise Exception("start_offset %d > %d" % ( valid_count, (1 << 10) - 1))
                    if valid_count >  (1 << 6) - 1: # in circle 999, valid_count max is 42
                        raise Exception("valid_count %d > %d" % ( valid_count, (1 << 6) - 1))
                    if data_value_offset > (1 << 16) - 1:
                        raise Exception("data_value_offset %d > %d" % ( data_value_offset, (1 << 16) - 1))
                    f.write(("    {.start_offset=%3d, .valid_count=%3d, .data_offset=%3d}, \n" % (start_offset, valid_count, data_value_offset)))

                    row_index += 1
                print('};', file=f)

                print('static const uint8_t %s_data_arr_sub_%d[%d] = {' % (name, i, len(data_value_array)), file=f)
                lineWidth=0
                for eachItem in data_value_array:
                    if lineWidth % 32 == (32 - 1):
                        print("0x%02x," %(eachItem) ,file=f)
                    else:
                        print("0x%02x" %(eachItem), end =", ",file=f)
                    lineWidth+=1
                print('', file=f)
                print('};', file=f)

            # insert next line
            print('', file=f)
                
            print('const egui_circle_info_t %s_arr[%d] = {' % (name, len(args.spec)),file=f)
            index = 0
            for i in args.spec:
                f.write(("    {.radius=%3d, .item_count=%3d, .items=%s_item_arr_sub_%d, .data=%s_data_arr_sub_%d}, \n" % (i, len(data_info_array_saved[index]), name, i, name, i)))
                index += 1
            print('};', file=f)

            len_spec = len(args.spec)
            print('const uint16_t %s_count_spec = %d;' % (name, len_spec),file=f)

            print(c_tail_string, file=f)
    else:
        # get the options
        options = f"-r {args.range}"

        name = f"egui_res_circle_info"
        outfilename = f"{name}.c"

        # need add 1.
        args.range += 1

        with open(outfilename,"w") as f:
            # insert header
            print(c_head_string, file=f)
            print(c_head_debug_string.format(args.range, options), file=f)

            data_info_array_saved = []
            # print the circle info
            for i in range(args.range):
                data_info_array, data_value_array = get_filled_circle_info(i)
                data_info_array_saved.append(data_info_array)

                
                print('#if (EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE >= %d)' % (i), file=f)
                print('static const egui_circle_item_t %s_item_arr_sub_%d[%d] = {' % (name, i, len(data_info_array)), file=f)
                row_index = 0
                for info in data_info_array:
                    start_offset, valid_count, data_value_offset = info
                    start_offset += row_index
                    if start_offset > (1 << 10) - 1:
                        raise Exception("start_offset %d > %d" % ( valid_count, (1 << 10) - 1))
                    if valid_count >  (1 << 6) - 1: # in circle 999, valid_count max is 42
                        raise Exception("valid_count %d > %d" % ( valid_count, (1 << 6) - 1))
                    if data_value_offset > (1 << 16) - 1:
                        raise Exception("data_value_offset %d > %d" % ( data_value_offset, (1 << 16) - 1))
                    f.write(("    {.start_offset=%3d, .valid_count=%3d, .data_offset=%3d}, \n" % (start_offset, valid_count, data_value_offset)))

                    row_index += 1
                print('};', file=f)

                print('static const uint8_t %s_data_arr_sub_%d[%d] = {' % (name, i, len(data_value_array)), file=f)
                lineWidth=0
                for eachItem in data_value_array:
                    if lineWidth % 32 == (32 - 1):
                        print("0x%02x," %(eachItem) ,file=f)
                    else:
                        print("0x%02x" %(eachItem), end =", ",file=f)
                    lineWidth+=1
                print('', file=f)
                print('};', file=f)
                print('#endif // (EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE >= %d)' % (i), file=f)

            # insert next line
            print('', file=f)
                
            print('const egui_circle_info_t %s_arr[EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE + 1] = {' % (name),file=f)
            for i in range(args.range):
                print('#if (EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE >= %d)' % (i), file=f)
                f.write(("    {.radius=%3d, .item_count=%3d, .items=%s_item_arr_sub_%d, .data=%s_data_arr_sub_%d}, \n" % (i, len(data_info_array_saved[i]), name, i, name, i)))
                print('#endif // (EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE >= %d)' % (i), file=f)
            print('};', file=f)

            print(c_tail_string, file=f)

if __name__ == '__main__':
    main(sys.argv[1:])
