import subprocess
import os
import shutil
import argparse

app_json_string="""
        {{
            "file": "{0}",
            "external": "{1}",
            "format": "{2}",
            "alpha": "{3}",
        }},"""


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


def mp4_to_images_ffmpeg(video_path, output_folder, target_fps, width_px, height_px, image_pre_name='frame', image_format='png'):
    # 检查输出文件夹是否存在，如果不存在则创建
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    print(f"Output folder: {output_folder}")
    scale_str = ''
    if width_px > 0 and height_px > 0:
        scale_str = f',scale={width_px}:{height_px}'
        pass

    # 构造 ffmpeg 命令
    command = [
        'ffmpeg',
        '-i', video_path,
        '-vf', f'fps={target_fps}{scale_str}',
        os.path.join(output_folder, f'{image_pre_name}_%04d.{image_format}')
    ]
    print(f"Executing command: {' '.join(command)}")
    # 执行 ffmpeg 命令
    subprocess.run(command, check=True)

    print("Frames extracted successfully.")

def generate_resource(video_path, output_folder, target_fps, width_px, height_px, format, alpha, external_type):
    video_name = os.path.basename(video_path)
    video_name = format_file_name(video_name.split('.')[0])
    image_pre_name = 'frame_' + video_name  # 图片文件名前缀

    # if not output_folder:
    #     output_folder = video_name

    # clear last output folder
    if(os.path.exists(output_folder)):
        shutil.rmtree(output_folder)

    mp4_to_images_ffmpeg(video_path, output_folder, target_fps, width_px, height_px, image_pre_name)

    # get image list
    image_list = os.listdir(output_folder)
    image_list.sort()

    last_path = os.path.basename(os.path.normpath(output_folder))

    # generate resource file
    resource_json_file = os.path.join(output_folder, video_name + '.json')
    
    with open(resource_json_file, 'w', encoding='utf-8') as f:
        for image_name in image_list:
            image_path = last_path + '/' + image_name
            
            f.write(app_json_string.format(image_path, external_type, format, alpha))

    print("Resource file generated successfully.")

    # generate c file
    resource_c_file = os.path.join(output_folder, video_name + '.h')
    with open(resource_c_file, 'w', encoding='utf-8') as f:
        image_total_count = len(image_list)
        define_image_total_count_name = f"MP4_IMAGE_COUNT_{video_name.upper()}"

        f.write(f"#define {define_image_total_count_name} {image_total_count}\n")
        f.write(f"extern const egui_image_t *mp4_arr_{video_name}[{define_image_total_count_name}];\n")
        f.write("\n")
        f.write("\n")
        f.write(f"const egui_image_t *mp4_arr_{video_name}[{define_image_total_count_name}] = \n")
        f.write("{\n")

        for image_file in image_list:
            image_name = format_file_name(image_file.split('.')[0])
            image_path = 'egui_res_image_' + last_path + '_' + image_name + '_' + format + '_' + str(alpha)
            if(external_type == 1):
                image_path += '_bin'
            f.write(f'    (const egui_image_t *)&{image_path},\n')
        f.write("};\n")

    print("C file generated successfully.")

def parse_args():
    parser = argparse.ArgumentParser(description='Generate resource files for egui example.')
    parser.add_argument('-i', "--input_file", nargs='?', type = str,  required=True, help="Resource path, such as xxx.mp4")
    parser.add_argument('-o', "--output", nargs='?', type = str,  required=True, help="Output path")
    parser.add_argument('-fps', "--fps", nargs='?', type = int,  required=True, help="Target frame rate")
    parser.add_argument('-f', '--format', nargs='?',type = str, default="rgb565", required=False, help="RGB Format (rgb32, rgb565, gray8)")
    parser.add_argument('-a', '--alpha', nargs='?',type = int, default=8, required=False, help="Alpha Format (0, 1, 2, 4, 8)")
    parser.add_argument('-ext', '--external', nargs='?',type = int, default=0, required=False, help="Storage format (0: internal, 1: external)")
    parser.add_argument('-width', '--width', nargs='?',type = int, default=0, required=False, help="Resize width (0: no resize)")
    parser.add_argument('-height', '--height', nargs='?',type = int, default=0, required=False, help="Resize height (0: no resize)")
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    generate_resource(args.input_file, args.output, args.fps, args.width, args.height, args.format, args.alpha, args.external)


