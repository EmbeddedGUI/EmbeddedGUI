import os
import shutil
import argparse
import json5
import ttf2c
import img2c


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
    if not os.path.exists(config_file_path):
        return None
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
        if self.external_info == 'all':
            self.external_list = [0, 1]
        else:
            self.external_list.append(int(self.external_info))
        
        self.dim = None
        if config.get('dim'):
            self.dim = [int(x) for x in config['dim'].split(',')]
        
        
        self.rot = 0.0
        if config.get('rot'):
            self.rot = float(config['rot'])
        
        self.swap = -1
        if config.get('swap'):
            self.swap = int(config['swap'])

class FontResourceInfo:
    def __init__(self, config):
        self.file_name = config['file']
        self.text_file_name = config['text']
        self.text_file_list = []
        self.text_file_list.append(self.text_file_name)

        self.pixelsize_info = 'all'
        if config.get('pixelsize'):
            self.pixelsize_info = config['pixelsize']

        self.pixelsize_list = []
        if self.pixelsize_info == 'all':
            # 4-32
            self.pixelsize_list = [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32]
        else:
            self.pixelsize_list.append(int(self.pixelsize_info))
        
        self.fontbitsize_info = 'all'
        if config.get('fontbitsize'):
            self.fontbitsize_info = config['fontbitsize']

        self.fontbitsize_list = []
        if self.fontbitsize_info == 'all':
            self.fontbitsize_list = [1, 2, 4, 8]
        else:
            self.fontbitsize_list.append(int(self.fontbitsize_info))
        
        
        self.external_info = 'all'
        if config.get('external'):
            self.external_info = config['external']
            
        self.external_list = []
        if self.external_info == 'all':
            self.external_list = [0, 1]
        else:
            self.external_list.append(int(self.external_info))

def generate_font_resource(resource_src_path, font_res_output_path, config_info_font):
    if not config_info_font:
        return
    # 检查重复的font配置
    font_config_list = []
    for font_config in config_info_font:
        # print(font_config)
        font_info = FontResourceInfo(font_config)
        for pixelsize in font_info.pixelsize_list:
            for fontbitsize in font_info.fontbitsize_list:
                for external in font_info.external_list:
                    # print(f"{font_info.file_name} {pixelsize} {fontbitsize} {external}")
                    is_conflit = False
                    for font_config_item in font_config_list:
                        if font_config_item[0].file_name == font_info.file_name and font_config_item[1] == pixelsize and font_config_item[2] == fontbitsize and font_config_item[3] == external:
                            is_conflit = True
                            font_config_item[0].text_file_list.append(font_info.text_file_name)
                            break
                    if is_conflit:
                        continue
                    font_config_list.append([font_info, pixelsize, fontbitsize, external])
    
    ttf2c_tool_list = []
    for font_config_item in font_config_list:
        # print(font_config_item)
        font_info = font_config_item[0]
        file_name = font_info.file_name
        font_file_path = os.path.join(resource_src_path, file_name)
        c_file_name = format_file_name(file_name.split('.')[0])

        output_path = font_res_output_path
        suported_text_list = []
        for text_file_path in font_info.text_file_list:
            suported_text_list.append(os.path.join(resource_src_path, text_file_path))
        font_name = c_file_name

        pixelsize = font_config_item[1]
        fontbitsize = font_config_item[2]
        external = font_config_item[3]
        tool = ttf2c.ttf2c_tool(font_file_path, font_name, suported_text_list, pixelsize, fontbitsize, external, output_path)

        ttf2c_tool_list.append(tool)

        print(f"Generating {tool.font_name} with {font_info.text_file_list}")

        tool.write_c_file()
        # generate_resource_font(ttf_util_path, font_res_output_path, font_file_path, c_file_name, text_file_path, font_config_item[1], font_config_item[2], font_config_item[3])
    return ttf2c_tool_list


def generate_img_resource(resource_src_path, img_res_output_path, config_info_img):
    if not config_info_img:
        return

    img_config_list = []
    for img_config in config_info_img:
        img_info = ImageResourceInfo(img_config)
        for rgb in img_info.rgb_list:
            for alpha in img_info.alpha_list:
                for external in img_info.external_list:
                    img_config_list.append([img_info, rgb, alpha, external, img_info.dim, img_info.rot, img_info.swap])
    
    img2c_tool_list = []
    for img_config_item in img_config_list:
        img_info = img_config_item[0]
        file_name = img_info.file_name
        img_file_path = os.path.join(resource_src_path, file_name)
        c_file_name = format_file_name(file_name.split('.')[0])

        img_name = c_file_name

        output_path = img_res_output_path
        rgb = img_config_item[1]
        alpha = img_config_item[2]
        external = img_config_item[3]
        dim = img_config_item[4]
        rot = img_config_item[5]
        swap = img_config_item[6]

        tool = img2c.img2c_tool(img_file_path, img_name, rgb, alpha, dim, rot, swap, external, output_path)

        img2c_tool_list.append(tool)

        print(f"Generating {tool.img_name}")

        tool.write_c_file()
        
    return img2c_tool_list

def generate_resource(resource_path, output_path, force):
    # 解析app_resource_config.json文件
    resource_src_path = os.path.join(resource_path, 'src')
    img_res_output_path = os.path.join(resource_path, 'img')
    font_res_output_path = os.path.join(resource_path, 'font')

    resource_bin_merge_file = os.path.join(resource_path, 'app_egui_resource_merge.bin')
    resource_bin_merge_file_output = os.path.join(output_path, 'app_egui_resource_merge.bin')
    app_egui_resource_generate_h_file_path = os.path.join(resource_path, 'app_egui_resource_generate.h')
    app_egui_resource_generate_c_file_path = os.path.join(resource_path, 'app_egui_resource_generate.c')
    if not force:
        if os.path.exists(resource_bin_merge_file):
            shutil.copy(resource_bin_merge_file, resource_bin_merge_file_output)
            return

        if os.path.exists(app_egui_resource_generate_h_file_path) and os.path.exists(app_egui_resource_generate_c_file_path):
            return

    if os.path.exists(resource_bin_merge_file):
        os.remove(resource_bin_merge_file)

    config_file_path = os.path.join(resource_src_path, 'app_resource_config.json')
    config_info = load_config_info(config_file_path)
    if config_info is None:
        print("app_resource_config.json is not exist")
        return

    # 先清除上一次的资源
    clear_last_resource(img_res_output_path)
    clear_last_resource(font_res_output_path)

    if not os.path.exists(img_res_output_path):
        os.makedirs(img_res_output_path)

    if not os.path.exists(font_res_output_path):
        os.makedirs(font_res_output_path)

    clear_last_resource(font_res_output_path)

    img2c_tool_list = generate_img_resource(resource_src_path, img_res_output_path, config_info['img'])
    ttf2c_tool_list = generate_font_resource(resource_src_path, font_res_output_path, config_info['font'])


    # 遍历生成app_egui_resource_generate.h文件
    resource_id_string = ""
    resource_extern_string = ""

    resource_bin_offset = 0
    resource_id_map_string = ""


    for tool in sorted(img2c_tool_list, key=lambda x: x.img_name):
        resource_extern_string += f"extern const egui_image_std_t {tool.img_name};\n"
        if tool.external_type:
            if tool.alpha_bin_data:
                id_str = tool.alpha_bin_name_res_id
                resource_id_string += id_str + ",\n"

                resource_id_map_string += f"0x{resource_bin_offset:08X}, // {id_str} \n"

                resource_bin_offset += len(tool.alpha_bin_data)

                # 通过添加写入到resource_bin_merge_file
                with open(resource_bin_merge_file, 'ab+') as target_file:
                    target_file.write(bytearray(tool.alpha_bin_data))


            id_str = tool.data_bin_name_res_id
            resource_id_string += id_str + ",\n"

            resource_id_map_string += f"0x{resource_bin_offset:08X}, // {id_str} \n"

            resource_bin_offset += len(tool.data_bin_data)

            # 通过添加写入到resource_bin_merge_file
            with open(resource_bin_merge_file, 'ab+') as target_file:
                target_file.write(bytearray(tool.data_bin_data))


    for tool in sorted(ttf2c_tool_list, key=lambda x: x.font_name):
        resource_extern_string += f"extern const egui_font_std_t {tool.font_name};\n"
        if tool.external_type:
            id_str = f"EGUI_EXT_RES_ID_{tool.font_name}".upper()
            resource_id_string += id_str + ",\n"

            resource_id_map_string += f"0x{resource_bin_offset:08X}, // {id_str} \n"

            resource_bin_offset += len(tool.data_bin_data)

            # 通过添加写入到resource_bin_merge_file
            with open(resource_bin_merge_file, 'ab+') as target_file:
                target_file.write(bytearray(tool.data_bin_data))

    # 生成app_egui_resource_generate.h文件
    print(f"Generating {app_egui_resource_generate_h_file_path}")
    with open(app_egui_resource_generate_h_file_path, 'w', encoding='utf-8') as f:
        f.write(app_egui_resource_generate_h_string.format(resource_extern_string, resource_id_string))


    # 生成app_egui_resource_generate.c文件
    print(f"Generating {app_egui_resource_generate_c_file_path}")
    with open(app_egui_resource_generate_c_file_path, 'w', encoding='utf-8') as f:
        f.write(app_egui_resource_generate_c_string.format(resource_id_map_string))


    # 拷贝bin文件到Output目录
    if os.path.exists(resource_bin_merge_file):
        shutil.copy(resource_bin_merge_file, resource_bin_merge_file_output)
    pass


def parse_args():
    parser = argparse.ArgumentParser(description='Generate resource files for egui example.')
    parser.add_argument('-r', "--resource", nargs='?', type = str,  required=True, help="Resource path")
    parser.add_argument('-o', "--output", nargs='?', type = str,  required=True, help="Output path")
    parser.add_argument('-f', "--force", nargs='?', type = bool,  required=False, default=False, help="Force to generate resource files")
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    generate_resource(args.resource, args.output, args.force)


