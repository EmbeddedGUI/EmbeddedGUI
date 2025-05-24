import os

def generate_mp4_image(script_path, input_video, target_fps, width_px, height_px, target_format, target_alpha, target_ext):
    output_path = input_video.split('.')[0]
    os.system("python " + script_path 
              + " -i " + input_video 
              + " -o " + output_path 
              + " -fps " + str(target_fps) 
              + " -width " + str(width_px) 
              + " -height " + str(height_px) 
              + " -f " + target_format 
              + " -a " + str(target_alpha) 
              + " -ext " + str(target_ext))

# basic param
script_path = "../../../"
script_path = os.path.join(script_path, "scripts/tools/app_mp4_image_generate.py")
# print(script_path)
target_fps = 10
target_width = 240 # change to size
target_height = 240 # change to size
target_format = "rgb565"
target_alpha = 0
target_ext = 0

input_video = "test.mp4"
generate_mp4_image(script_path, input_video, target_fps, target_width, target_height, target_format, target_alpha, target_ext)
