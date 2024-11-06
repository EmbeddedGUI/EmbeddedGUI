import os
import shutil
import argparse
import json5


app_egui_resource_generate_h_string="""

#ifndef _APP_EGUI_RESOURCE_GENERATE_H_
#define _APP_EGUI_RESOURCE_GENERATE_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {{
#endif

#include "egui.h"

{0}

enum {{
EGUI_EXT_RES_ID_BASE = 0x00, // avoid conflict with NULL.
{1}
EGUI_EXT_RES_ID_MAX,
}};

extern const uint32_t egui_ext_res_id_map[EGUI_EXT_RES_ID_MAX];

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}}
#endif

#endif /* _APP_EGUI_RESOURCE_GENERATE_H_ */

"""


app_egui_resource_generate_c_string="""
#include "app_egui_resource_generate.h"


const uint32_t egui_ext_res_id_map[EGUI_EXT_RES_ID_MAX] = {{
0x00000000, // EGUI_EXT_RES_ID_BASE
{0}
}};


"""

def format_file_name(file_name):
    file_name = file_name.replace('-', '_')
    file_name = file_name.replace('.', '_')
    file_name = file_name.replace('(', '_')
    file_name = file_name.replace(')', '_')
    file_name = file_name.replace(',', '_')
    file_name = file_name.replace(' ', '_')
    # avoid too long file name
    file_name = file_name.replace('__', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.lower()
    return file_name

# print("Generating 8 px")
# os.system("python ../ttf2c.py -i Montserrat-Medium.ttf -n montserrat -t supported_text.txt -p 8 -s 4")

def generate_resource_font(font_path, font_file_name, font_name, output_path):
    ttf_util_path = ('scripts/tools/ttf2c.py')
    font_file_path = os.path.join(font_path, font_file_name)
    suported_text_path = os.path.join(output_path,'supported_text.txt')
    for px in range(6, 60):
        for alpha in [1, 2, 4, 8]:
            os.system(f"python {ttf_util_path} -i \"{font_file_path}\" -n {font_name} -t {suported_text_path} -p {px} -s {alpha} -o {output_path}")
    pass
# os.system("python ../img2c.py -i star.png -n star -f rgb565 -a 8 -d 50 50")
# 
def generate_resource_img(img_util_path, img_res_output_path, img_file_path, img_name, rgb_list, alpha_list, external_list, dim, rot, swap):
    if not os.path.exists(img_res_output_path):
        os.makedirs(img_res_output_path)
    for rgb in rgb_list:
        for alpha in alpha_list:
            for external in external_list:

                print(f"Generating {img_name}_{rgb}_{alpha}px")

                # output_path = os.path.join(img_res_output_path, f"{rgb}_{alpha}")
                output_path = img_res_output_path
                if not os.path.exists(output_path):
                    os.makedirs(output_path)
                cmd = f"python {img_util_path} -i \"{img_file_path}\" -n {img_name} -f {rgb} -a {alpha} -o {output_path}"

                if dim[0] != -1 and dim[1]:
                    cmd += f" -d {dim[0]} {dim[1]}"
                if rot != -1:
                    cmd += f" -r {rot}"
                if swap != -1:
                    cmd += f" -s {swap}"

                if external == 1:
                    cmd += " -ext 1"

                print(cmd)
                os.system(cmd)
    pass

def clear_last_resource(resource_path):
    if os.path.exists(resource_path):
        # delete old resource
        for file in os.listdir(resource_path):
            file_path = os.path.join(resource_path, file)
            if os.path.isfile(file_path):
                os.remove(file_path)
            else:
                shutil.rmtree(file_path)

def load_config_info(config_file_path):
    with open(config_file_path, 'r', encoding='utf-8') as f:
        return json5.load(f)
    return None

class ImageResourceInfo:
    def __init__(self, config):
        self.file_name = config['file']

        self.rgb_info = 'all'
        if config.get('format'):
            self.rgb_info = config['format']

        self.alpha_info = 'all'
        if config.get('alpha'):
            self.alpha_info = config['alpha']

        self.rgb_list = []
        if self.rgb_info == 'all':
            self.rgb_list = ['rgb32', 'rgb565', 'gray8']
        else:
            self.rgb_list.append(self.rgb_info)
        
        self.alpha_list = []
        if self.alpha_info == 'all':
            self.alpha_list = [0, 1, 2, 4, 8]
        else:
            self.alpha_list.append(int(self.alpha_info))

        
        self.alpha_list = []
        if self.alpha_info == 'all':
            self.alpha_list = [0, 1, 2, 4, 8]
        else:
            self.alpha_list.append(int(self.alpha_info))
        
        
        self.external_info = 'all'
        if config.get('external'):
            self.external_info = config['external']
            
        self.external_list = []
        if self.alpha_info == 'all':
            self.external_list = [0, 1]
        else:
            self.external_list.append(int(self.external_info))
        
        self.dim = [-1, -1]
        if config.get('dim'):
            self.dim = [int(x) for x in config['dim'].split(',')]
        
        
        self.rot = -1
        if config.get('rot'):
            self.rot = float(config['rot'])
        
        self.swap = -1
        if config.get('swap'):
            self.swap = int(config['swap'])


def generate_resource(resource_path, output_path):
    # 解析app_resource_config.json文件
    config_file_path = os.path.join(resource_path, 'app_resource_config.json')
    config_info = load_config_info(config_file_path)
    if config_info is None:
        print("app_resource_config.json is not exist")
        return

    # 先清除上一次的资源
    img_res_output_path = os.path.join(resource_path, 'img')
    clear_last_resource(img_res_output_path)

    img_util_path = ('scripts/tools/img2c.py')

    font_res_output_path = os.path.join(resource_path, 'font')
    clear_last_resource(font_res_output_path)

    resource_bin_merge_file = os.path.join(resource_path, 'app_egui_resource_merge.bin')
    resource_bin_merge_file_output = os.path.join(output_path, 'app_egui_resource_merge.bin')
    if os.path.exists(resource_bin_merge_file):
        os.remove(resource_bin_merge_file)

    config_info_img = config_info['img']
    if config_info_img:
        for img_config in config_info_img:
            img_info = ImageResourceInfo(img_config)
            file_name = img_info.file_name
            img_file_path = os.path.join(resource_path, file_name)
            c_file_name = format_file_name(file_name.split('.')[0])

            generate_resource_img(img_util_path, img_res_output_path, img_file_path, c_file_name, img_info.rgb_list, img_info.alpha_list, img_info.external_list, img_info.dim, img_info.rot, img_info.swap)

    # 遍历路径根目录下所有图片文件，只找根目录的文件
    # for file in os.listdir(resource_path):
    #     if file.endswith('.png') or file.endswith('.jpg') or file.endswith('.jpeg') or file.endswith('.bmp'):
    #         c_file_name = format_file_name(file.split('.')[0])
    #         img_file_path = os.path.join(resource_path, file)
    #         generate_resource_img(img_util_path, img_res_output_path, img_file_path, c_file_name)
        # elif file.endswith('.ttf') or file.endswith('.otf'):
        #     generate_resource_font(root, file, file.split('.')[0], resource_path)


    # 遍历生成app_egui_resource_generate.h文件
    resource_id_string = ""
    resource_extern_string = ""

    resource_bin_offset = 0
    resource_id_map_string = ""

    # 遍历img_res_output_path目录下所有c文件，包括子目录
    for root, dirs, files in os.walk(img_res_output_path):
        for file in files:
            if file.endswith('.c'):
                resource_extern_string += f"extern const egui_image_std_t {file.split('.')[0]};\n"
            elif file.endswith('.bin'):
                id_str = f"EGUI_EXT_RES_ID_{file.split('.')[0]}".upper()
                resource_id_string += id_str + ",\n"

                resource_id_map_string += f"0x{resource_bin_offset:08X}, // {id_str} \n"

                file_path = os.path.join(root, file)
                with open(file_path, 'rb') as bin_file:
                    content = bin_file.read()
                    resource_bin_offset += len(content)

                    # 通过添加写入到resource_bin_merge_file
                    with open(resource_bin_merge_file, 'ab+') as target_file:
                        target_file.write(content)


    # 生成app_egui_resource_generate.h文件
    app_egui_resource_generate_h_file_path = os.path.join(resource_path, 'app_egui_resource_generate.h')
    print(f"Generating {app_egui_resource_generate_h_file_path}")
    with open(app_egui_resource_generate_h_file_path, 'w', encoding='utf-8') as f:
        f.write(app_egui_resource_generate_h_string.format(resource_extern_string, resource_id_string))


    # 生成app_egui_resource_generate.c文件
    app_egui_resource_generate_c_file_path = os.path.join(resource_path, 'app_egui_resource_generate.c')
    print(f"Generating {app_egui_resource_generate_c_file_path}")
    with open(app_egui_resource_generate_c_file_path, 'w', encoding='utf-8') as f:
        f.write(app_egui_resource_generate_c_string.format(resource_id_map_string))


    # 拷贝bin文件到Output目录
    shutil.copy(resource_bin_merge_file, resource_bin_merge_file_output)
    pass


def parse_args():
    parser = argparse.ArgumentParser(description='Generate resource files for egui example.')
    parser.add_argument('-r', "--resource", nargs='?', type = str,  required=True, help="Resource path")
    parser.add_argument('-o', "--output", nargs='?', type = str,  required=True, help="Output path")
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    generate_resource(args.resource, args.output)

