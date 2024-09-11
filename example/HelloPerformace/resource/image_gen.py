#!/usr/bin/env python3

import os

def generate_all_format(img, width, height):
    img_format_list = ["rgb565", "rgb32"]
    alpha_list = [0, 1, 2, 4, 8]
    # 获取文件名称
    name = os.path.basename(img).split(".")[0]

    for img_format in img_format_list:
        for alpha in alpha_list:
            if img_format == "rgb32":
                if alpha != 8:
                    continue
            options = "-i %s -n %s -f %s -a %s -d %s %s" % (img, name, img_format, alpha, width, height)
            cmd = "python scripts/tools/img2c.py %s" % options
            print(cmd)
            os.system(cmd)

if __name__ == "__main__":
    current_file_path = os.path.abspath(__file__)
    current_directory_path = os.path.dirname(current_file_path)

    img = os.path.join(current_directory_path, "test_img_0_960_1280.png")
    generate_all_format(img, 240, 320)
    img = os.path.join(current_directory_path, "test_img_1_1280_960.png")
    generate_all_format(img, 320, 240)
    img = os.path.join(current_directory_path, "test_img_2_1280_1280.png")
    generate_all_format(img, 240, 240)
    img = os.path.join(current_directory_path, "test_img_3_1280_960.png")
    generate_all_format(img, 320, 240)
    img = os.path.join(current_directory_path, "test_img_4_1280_1280.png")
    generate_all_format(img, 240, 240)
    img = os.path.join(current_directory_path, "test_img_5_1280_1280.png")
    generate_all_format(img, 240, 240)
    img = os.path.join(current_directory_path, "test_img_6_1280_688.png")
    generate_all_format(img, 320, 172)

    img = os.path.join(current_directory_path, "star.png")
    generate_all_format(img, 150, 150)

