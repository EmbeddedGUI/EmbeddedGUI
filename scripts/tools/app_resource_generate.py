import os
import shutil
import argparse
import subprocess
import json5
import ttf2c
import img2c


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_IN_DIR = os.path.join(SCRIPT_DIR, 'build_in')


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
    file_name = file_name.replace('//', '_')
    file_name = file_name.replace('/', '_')
    file_name = file_name.replace('\\', '_')
    # avoid too long file name
    file_name = file_name.replace('__', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.lower()
    return file_name


def resolve_input_file_path(resource_src_path, file_name):
    if os.path.isabs(file_name):
        return os.path.normpath(file_name)

    normalized_file_name = file_name.replace('\\', '/')
    if normalized_file_name.startswith('build_in/'):
        build_in_relative_path = normalized_file_name[len('build_in/'):]
        return os.path.normpath(os.path.join(BUILD_IN_DIR, build_in_relative_path))

    return os.path.normpath(os.path.join(resource_src_path, file_name))


def make_report_link(resource_path, input_file_path):
    relative_path = os.path.relpath(os.path.normpath(input_file_path), resource_path)
    return relative_path.replace('\\', '/')


def clear_last_resource(resource_path):
    if os.path.exists(resource_path):
        # delete old resource
        for file in os.listdir(resource_path):
            file_path = os.path.join(resource_path, file)
            if os.path.isfile(file_path):
                os.remove(file_path)
            else:
                shutil.rmtree(file_path)


def ensure_output_bin_file(output_bin_path):
    if not output_bin_path:
        return
    output_dir = os.path.dirname(output_bin_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)
    with open(output_bin_path, 'wb'):
        pass

def load_config_info(config_file_path):
    if not os.path.exists(config_file_path):
        return None
    with open(config_file_path, 'r', encoding='utf-8') as f:
        return json5.load(f)
    return None

class ImageResourceInfo:
    def __init__(self, config):
        self.file_name = config['file']
        self.name = None
        if config.get('name'):
            self.name = config['name']
        else:
            c_file_name = format_file_name(self.file_name.split('.')[0])
            self.name = c_file_name

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

        self.bg = None
        if config.get('bg'):
            self.bg = config['bg']

        self.compress = "none"
        if config.get('compress'):
            self.compress = config['compress']

class FontResourceInfo:
    def __init__(self, config):
        self.file_name = config['file']
        self.name = None
        if config.get('name'):
            self.name = config['name']
        else:
            c_file_name = format_file_name(self.file_name.split('.')[0])
            self.name = c_file_name
                

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

        self.weight = None
        if config.get('weight'):
            self.weight = int(config['weight'])

def generate_font_resource(resource_src_path, font_res_output_path, config_info_font):
    ttf2c_tool_list = []
    if not config_info_font:
        return ttf2c_tool_list
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
                        if font_config_item[0].name == font_info.name and font_config_item[1] == pixelsize and font_config_item[2] == fontbitsize and font_config_item[3] == external:
                            is_conflit = True
                            font_config_item[0].text_file_list.append(font_info.text_file_name)
                            break
                    if is_conflit:
                        continue
                    font_config_list.append([font_info, pixelsize, fontbitsize, external])
    
    for font_config_item in font_config_list:
        # print(font_config_item)
        font_info = font_config_item[0]
        file_name = font_info.file_name
        font_file_path = resolve_input_file_path(resource_src_path, file_name)
        c_file_name = format_file_name(file_name.split('.')[0])

        output_path = font_res_output_path
        suported_text_list = []
        for text_file_path in font_info.text_file_list:
            suported_text_list.append(os.path.join(resource_src_path, text_file_path))
        font_name = font_info.name

        pixelsize = font_config_item[1]
        fontbitsize = font_config_item[2]
        external = font_config_item[3]
        tool = ttf2c.ttf2c_tool(font_file_path, font_name, suported_text_list, pixelsize, fontbitsize, external, output_path, font_info.weight)

        ttf2c_tool_list.append(tool)

        print(f"Generating {tool.font_name} with {font_info.text_file_list}")

        tool.write_c_file()
        # generate_resource_font(ttf_util_path, font_res_output_path, font_file_path, c_file_name, text_file_path, font_config_item[1], font_config_item[2], font_config_item[3])
    return ttf2c_tool_list


def generate_img_resource(resource_src_path, img_res_output_path, config_info_img):
    img2c_tool_list = []
    if not config_info_img:
        return img2c_tool_list

    img_config_list = []
    for img_config in config_info_img:
        img_info = ImageResourceInfo(img_config)
        # Validate: GRAY8 + QOI is not supported (QOI encodes RGB/RGBA only)
        if img_info.compress == 'qoi':
            for rgb in img_info.rgb_list:
                if rgb == 'gray8':
                    print(f"[WARN] Image '{img_info.name}': QOI does not support GRAY8 format, skipping QOI for gray8 variants.")
        for rgb in img_info.rgb_list:
            for alpha in img_info.alpha_list:
                for external in img_info.external_list:
                    compress = img_info.compress
                    # Skip GRAY8 + QOI combination
                    if compress == 'qoi' and rgb == 'gray8':
                        compress = 'none'
                    img_config_list.append([img_info, rgb, alpha, external, img_info.dim, img_info.rot, img_info.bg, compress])
    
    for img_config_item in img_config_list:
        img_info = img_config_item[0]
        file_name = img_info.file_name
        img_file_path = os.path.join(resource_src_path, file_name)
        c_file_name = format_file_name(file_name.split('.')[0])

        img_name = img_info.name

        output_path = img_res_output_path
        rgb = img_config_item[1]
        alpha = img_config_item[2]
        external = img_config_item[3]
        dim = img_config_item[4]
        rot = img_config_item[5]
        bg = img_config_item[6]
        compress = img_config_item[7]

        tool = img2c.img2c_tool(img_file_path, img_name, rgb, alpha, dim, rot, 0, external, output_path, bg, compress=compress)

        img2c_tool_list.append(tool)

        print(f"Generating {tool.img_name}")

        tool.write_c_file()
        
    return img2c_tool_list


def _mp4_extract_frames(video_path, output_folder, target_fps, width_px, height_px, image_pre_name='frame', image_format='png'):
    """Extract frames from an MP4 video using ffmpeg."""
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    scale_str = ''
    if width_px > 0 and height_px > 0:
        scale_str = f',scale={width_px}:{height_px}'
    command = [
        'ffmpeg',
        '-i', video_path,
        '-vf', f'fps={target_fps}{scale_str}',
        os.path.join(output_folder, f'{image_pre_name}_%04d.{image_format}')
    ]
    subprocess.run(command, check=True)


def generate_mp4_resource(resource_src_path, img_res_output_path, mp4_header_output_path, config_info_mp4):
    """Process 'mp4' entries in config: extract frames, generate img resources, create frame array headers.

    Each mp4 entry specifies:
        file:     Source video file (e.g., "test.mp4")
        name:     Unique name for this frame set (defaults to video basename)
        fps:      Frame extraction rate (default: 10)
        width:    Target frame width (0 = no resize)
        height:   Target frame height (0 = no resize)
        format:   RGB format (default: "rgb565")
        alpha:    Alpha format (default: "0")
        external: Storage format (default: "0")
        compress: Compression codec (default: "none")

    Workflow:
        1. Extract frames from video using ffmpeg (skipped if frames already exist)
        2. Generate img2c resources for each frame
        3. Generate C header with frame pointer array

    Returns: (all_mp4_tools, mp4_groups) where mp4_groups = [(name, tools), ...]
    """
    all_mp4_tools = []
    mp4_groups = []

    if not config_info_mp4:
        return all_mp4_tools, mp4_groups

    extracted_videos = {}

    for mp4_config in config_info_mp4:
        video_file = mp4_config['file']
        video_path = os.path.join(resource_src_path, video_file)
        video_base_name = format_file_name(os.path.splitext(os.path.basename(video_file))[0])

        fps = int(mp4_config.get('fps', 10))
        width = int(mp4_config.get('width', 0))
        height = int(mp4_config.get('height', 0))
        fmt = mp4_config.get('format', 'rgb565')
        alpha = str(mp4_config.get('alpha', '0'))
        external = str(mp4_config.get('external', '0'))
        compress = mp4_config.get('compress', 'none')
        name = mp4_config.get('name', video_base_name)

        frame_dir_name = video_base_name
        frame_dir = os.path.join(resource_src_path, frame_dir_name)
        frame_prefix = f'frame_{video_base_name}'

        # Extract frames once per video+fps+size combination
        extract_key = f"{video_file}_{fps}_{width}_{height}"
        if extract_key not in extracted_videos:
            existing_frames = []
            if os.path.exists(frame_dir):
                existing_frames = [f for f in os.listdir(frame_dir) if f.lower().endswith(('.png', '.jpg'))]

            if not existing_frames:
                if os.path.exists(video_path):
                    try:
                        _mp4_extract_frames(video_path, frame_dir, fps, width, height, frame_prefix)
                    except Exception as e:
                        print(f"[WARN] Failed to extract MP4 frames: {e}")
                        print(f"[INFO] Place pre-extracted PNG frames in: {frame_dir}")
                else:
                    print(f"[INFO] MP4 source not found: {video_path}")
                    print(f"[INFO] Place pre-extracted PNG frames in: {frame_dir}")
            extracted_videos[extract_key] = frame_dir_name

        # List available frames
        if not os.path.exists(frame_dir):
            print(f"[WARN] MP4 frame directory not found: {frame_dir}")
            continue

        frame_files = sorted([f for f in os.listdir(frame_dir) if f.lower().endswith(('.png', '.jpg'))])
        if not frame_files:
            print(f"[WARN] No frame images found in: {frame_dir}")
            continue

        # Generate img config entries for each frame
        img_configs = []
        for frame_file in frame_files:
            # Build a unique name per mp4 entry to avoid conflicts when
            # multiple entries share the same source frames (e.g. different sizes).
            frame_base = format_file_name(os.path.splitext(frame_file)[0])
            unique_name = f'{name}_{frame_base}'
            entry = {
                'file': f'{frame_dir_name}/{frame_file}',
                'name': unique_name,
                'format': fmt,
                'alpha': alpha,
                'external': external,
            }
            if compress and compress != 'none':
                entry['compress'] = compress
            if width > 0 and height > 0:
                entry['dim'] = f'{width},{height}'
            img_configs.append(entry)

        # Process through img pipeline
        tools = generate_img_resource(resource_src_path, img_res_output_path, img_configs)
        all_mp4_tools.extend(tools)
        mp4_groups.append((name, tools))

        print(f"[MP4] {name}: {len(tools)} frames, compress={compress}")

    # Generate frame array headers
    for group_name, tools in mp4_groups:
        header_path = os.path.join(mp4_header_output_path, f'app_egui_resource_mp4_{group_name}.h')
        _generate_mp4_frame_header(group_name, tools, header_path)

    return all_mp4_tools, mp4_groups


def _generate_mp4_frame_header(name, tools, header_path):
    """Generate a C header with MP4 frame count define and frame pointer array."""
    guard = f"_APP_EGUI_RESOURCE_MP4_{name.upper()}_H_"
    count_define = f"MP4_IMAGE_COUNT_{name.upper()}"
    arr_name = f"mp4_arr_{name}"

    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(f"/* Auto-generated MP4 frame array for '{name}' */\n")
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write(f"#include \"app_egui_resource_generate.h\"\n\n")
        f.write(f"#define {count_define} {len(tools)}\n\n")
        f.write(f"static const egui_image_t *{arr_name}[{count_define}] =\n")
        f.write("{\n")
        for tool in sorted(tools, key=lambda x: x.img_name):
            f.write(f"    (const egui_image_t *)&{tool.img_name},\n")
        f.write("};\n\n")
        f.write(f"#endif /* {guard} */\n")

    print(f"Generated MP4 header: {header_path}")


def generate_resource(resource_path, output_path, force, output_bin_path=None):
    # 解析app_resource_config.json文件
    resource_src_path = os.path.join(resource_path, 'src')
    img_res_output_path = os.path.join(resource_path, 'img')
    font_res_output_path = os.path.join(resource_path, 'font')

    resource_bin_merge_file = os.path.join(resource_path, 'app_egui_resource_merge.bin')
    resource_bin_merge_file_output = output_bin_path or os.path.join(output_path, 'app_egui_resource_merge.bin')
    app_egui_resource_generate_h_file_path = os.path.join(resource_path, 'app_egui_resource_generate.h')
    app_egui_resource_generate_c_file_path = os.path.join(resource_path, 'app_egui_resource_generate.c')

    app_egui_resource_generate_report_file_path = os.path.join(resource_path, 'app_egui_resource_generate_report.md')
    if not force:
        if os.path.exists(resource_bin_merge_file):
            shutil.copy(resource_bin_merge_file, resource_bin_merge_file_output)
            return

        if os.path.exists(app_egui_resource_generate_h_file_path) and os.path.exists(app_egui_resource_generate_c_file_path):
            ensure_output_bin_file(resource_bin_merge_file_output)
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

    # Process MP4 entries (frame extraction + img resource generation + frame array headers)
    mp4_tool_list, mp4_groups = generate_mp4_resource(
        resource_src_path, img_res_output_path, resource_path, config_info.get('mp4', []))
    img2c_tool_list.extend(mp4_tool_list)

    # 遍历生成app_egui_resource_generate.h文件
    resource_id_string = ""
    resource_extern_string = ""

    resource_bin_offset = 0
    resource_id_map_string = ""

    for tool in sorted(img2c_tool_list, key=lambda x: x.img_name):
        if tool.compress == "rle":
            img_type = "egui_image_rle_t"
        elif tool.compress == "qoi":
            img_type = "egui_image_qoi_t"
        else:
            img_type = "egui_image_std_t"
        resource_extern_string += f"extern const {img_type} {tool.img_name};\n"
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
            id_str = tool.pixel_buffer_bin_name_res_id
            resource_id_string += id_str + ",\n"

            resource_id_map_string += f"0x{resource_bin_offset:08X}, // {id_str} \n"

            resource_bin_offset += len(tool.pixel_buffer_bin_data)

            # 通过添加写入到resource_bin_merge_file
            with open(resource_bin_merge_file, 'ab+') as target_file:
                target_file.write(bytearray(tool.pixel_buffer_bin_data))

                
            id_str = tool.char_desc_bin_name_res_id
            resource_id_string += id_str + ",\n"

            resource_id_map_string += f"0x{resource_bin_offset:08X}, // {id_str} \n"

            resource_bin_offset += len(tool.char_desc_bin_data)

            # 通过添加写入到resource_bin_merge_file
            with open(resource_bin_merge_file, 'ab+') as target_file:
                target_file.write(bytearray(tool.char_desc_bin_data))

    # 生成app_egui_resource_generate.h文件
    print(f"Generating {app_egui_resource_generate_h_file_path}")
    with open(app_egui_resource_generate_h_file_path, 'w', encoding='utf-8') as f:
        f.write(app_egui_resource_generate_h_string.format(resource_extern_string, resource_id_string))


    # 生成app_egui_resource_generate.c文件
    print(f"Generating {app_egui_resource_generate_c_file_path}")
    with open(app_egui_resource_generate_c_file_path, 'w', encoding='utf-8') as f:
        f.write(app_egui_resource_generate_c_string.format(resource_id_map_string))




    # 生成报告
    print(f"Generating {app_egui_resource_generate_report_file_path}")
    with open(app_egui_resource_generate_report_file_path, 'w', encoding='utf-8') as f:
        img_data_total_size = 0
        img_alpha_total_size = 0
        img_total_size = 0

        img_ext_data_total_size = 0
        img_ext_alpha_total_size = 0
        img_ext_total_size = 0

        font_total_size = 0
        font_ext_total_size = 0
        f.write("# 图像\n")
        f.write("## 内部\n")
        f.write("| 名称 | data大小 | alpha大小 | 总大小 | 压缩 | Image |\n")
        f.write("| ---- | -------- | --------- | ------ | ---- | ------ |\n")
        for tool in sorted(img2c_tool_list, key=lambda x: x.img_name):
            if tool.external_type:
                continue
            compress_info = ""
            if tool.compress != "none" and (tool.compressed_data_size > 0 or tool.compressed_alpha_size > 0):
                tmp_data_size = tool.compressed_data_size
                tmp_alpha_size = tool.compressed_alpha_size
                tmp_total = tmp_data_size + tmp_alpha_size
                # Calculate original (uncompressed) size
                orig_data = len(tool.data_bin_data) if tool.data_bin_data else 0
                orig_alpha = len(tool.alpha_bin_data) if tool.alpha_bin_data else 0
                orig_total = orig_data + orig_alpha
                if orig_total > 0:
                    ratio = (1 - tmp_total / orig_total) * 100
                    compress_info = f"{tool.compress.upper()} (-{ratio:.0f}%, 原始{orig_total})"
                else:
                    compress_info = tool.compress.upper()
            else:
                tmp_data_size = len(tool.data_bin_data)
                tmp_alpha_size = 0
                if tool.alpha_bin_data:
                    tmp_alpha_size = len(tool.alpha_bin_data)
            img_data_total_size += tmp_data_size
            img_alpha_total_size += tmp_alpha_size
            img_total_size += tmp_data_size + tmp_alpha_size
            # f"![{tool.img_name}](src/{tool.filename})"
            img_file_path = tool.input_img_file
            # 只保留img_file_path中src之后的路径信息
            if 'src/' in img_file_path:
                img_file_path = img_file_path.split('src/')[1]
            elif 'src\\' in img_file_path:
                img_file_path = img_file_path.split('src\\')[1]
                
            f.write(f"| {tool.img_name} | {tmp_data_size} | {tmp_alpha_size} | {tmp_data_size + tmp_alpha_size} | {compress_info} | ![{tool.img_name}](src/{img_file_path}) |\n")
        f.write(f"| 总计 | {img_data_total_size} | {img_alpha_total_size} | {img_total_size} | | |\n")

        f.write("\n")
        f.write("\n")
        f.write("\n")
        f.write("## 外部\n")
        f.write("| 名称 | data大小 | alpha大小 | 总大小 | Image |\n")
        f.write("| ---- | -------- | --------- | ------ | ------ |\n")
        for tool in sorted(img2c_tool_list, key=lambda x: x.img_name):
            if not tool.external_type:
                continue
            tmp_data_size = len(tool.data_bin_data)
            tmp_alpha_size = 0
            if tool.alpha_bin_data:
                tmp_alpha_size = len(tool.alpha_bin_data)
            img_ext_data_total_size += tmp_data_size
            img_ext_alpha_total_size += tmp_alpha_size
            img_ext_total_size += tmp_data_size + tmp_alpha_size
            f.write(f"| {tool.img_name} | {tmp_data_size} | {tmp_alpha_size} | {tmp_data_size + tmp_alpha_size} | ![{tool.img_name}](src/{tool.filename}) |\n")
        f.write(f"| 总计 | {img_ext_data_total_size} | {img_ext_alpha_total_size} | {img_ext_total_size} |\n")

# [tttt](src/supported_text_title.txt)
        f.write("\n")
        f.write("\n")
        f.write("\n")
        f.write("# 字体\n")
        f.write("## 内部\n")
        f.write("| 名称 | 总大小 | Font | Text |\n")
        f.write("| ---- | ------ | ------ | ------ |\n")
        for tool in sorted(ttf2c_tool_list, key=lambda x: x.font_name):
            if tool.external_type:
                continue
            tmp_pixel_size = len(tool.pixel_buffer_bin_data)
            tmp_char_size = len(tool.char_desc_bin_data)
            font_total_size += tmp_pixel_size + tmp_char_size
            font_report_path = make_report_link(resource_path, tool.input_font_file)
            font_file_string = f"[{tool.input_font_file_name}]({font_report_path})"

            text_file_string = ""
            for file in tool.text_file:
                text_file_name = os.path.basename(file)
                text_file_string += f"[{text_file_name}](src/{text_file_name})"
                text_file_string += " "
            f.write(f"| {tool.font_name} | {tmp_pixel_size + tmp_char_size} | {font_file_string} | {text_file_string} |\n")
        f.write(f"| 总计 | {font_total_size} |\n")

        f.write("\n")
        f.write("\n")
        f.write("\n")
        f.write("## 外部\n")
        f.write("| 名称 | 总大小 |\n")
        f.write("| ---- | ------ |\n")
        for tool in sorted(ttf2c_tool_list, key=lambda x: x.font_name):
            if not tool.external_type:
                continue
            tmp_pixel_size = len(tool.pixel_buffer_bin_data)
            tmp_char_size = len(tool.char_desc_bin_data)
            font_ext_total_size += tmp_pixel_size + tmp_char_size
            f.write(f"| {tool.font_name} | {tmp_pixel_size + tmp_char_size} |\n")
        f.write(f"| 总计 | {font_ext_total_size} |\n")



        f.write("# 总计\n")
        f.write("| 名称 | 总大小 |\n")
        f.write("| ---- | ------ |\n")
        f.write(f"| 图像-内部 | {img_total_size} |\n")
        f.write(f"| 字体-内部 | {font_total_size} |\n")
        f.write(f"| 内部总计 | {img_total_size + font_total_size} |\n")
        f.write(f"| 图像-外部 | {img_ext_total_size} |\n")
        f.write(f"| 字体-外部 | {font_ext_total_size} |\n")
        f.write(f"| 外部总计 | {img_ext_total_size + font_ext_total_size} |\n")
        f.write(f"| 总计 | {img_ext_total_size + font_ext_total_size + img_total_size + font_total_size} |\n")


        total_report_info = ""
        total_report_info += f"{'内部'.center(32, '=')}\n"
        total_report_info += f"{'Image'.center(10)} {'Font'.center(10)} {'Total'.center(10)}\n"
        total_report_info += f"{str(img_total_size).center(10)} {str(font_total_size).center(10)} {str(img_total_size + font_total_size).center(10)}\n"
        total_report_info += f"{'外部'.center(32, '=')}\n"
        total_report_info += f"{'Image'.center(10)} {'Font'.center(10)} {'Total'.center(10)}\n"
        total_report_info += f"{str(img_ext_total_size).center(10)} {str(font_ext_total_size).center(10)} {str(img_ext_total_size + font_ext_total_size).center(10)}\n"
        total_report_info += f"{'总计'.center(32, '=')}\n"
        total_report_info += f"{'Image'.center(10)} {'Font'.center(10)} {'Total'.center(10)}\n"
        total_report_info += f"{str(img_ext_total_size + img_total_size).center(10)} {str(font_ext_total_size + font_total_size).center(10)} {str(img_ext_total_size + font_ext_total_size + img_total_size + font_total_size).center(10)}\n"
        print(total_report_info)






    # 拷贝bin文件到Output目录
    if os.path.exists(resource_bin_merge_file):
        shutil.copy(resource_bin_merge_file, resource_bin_merge_file_output)
    else:
        ensure_output_bin_file(resource_bin_merge_file_output)
    pass


def parse_args():
    parser = argparse.ArgumentParser(description='Generate resource files for egui example.')
    parser.add_argument('-r', "--resource", nargs='?', type = str,  required=True, help="Resource path")
    parser.add_argument('-o', "--output", nargs='?', type = str,  required=True, help="Output path")
    parser.add_argument('-f', "--force", nargs='?', type = bool,  required=False, default=False, help="Force to generate resource files")
    parser.add_argument("--output-bin-path", nargs='?', type=str, required=False, default=None, help="Override merged resource bin output path")
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    generate_resource(args.resource, args.output, args.force, args.output_bin_path)


