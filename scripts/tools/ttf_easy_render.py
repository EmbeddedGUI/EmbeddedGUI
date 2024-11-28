import freetype
import numpy as np
from PIL import Image
 
def render_text(text, font_filename, size, img_width=600, img_height=400, output_filename='output.png'):
    # 初始化FreeType库
    ft_face = freetype.Face(font_filename)
    ft_face.set_char_size(size * 64)  # 设置字体大小
    # ft_face.set_pixel_sizes(size)

    ascender = ft_face.size.ascender / 64.0
    descender = ft_face.size.descender / 64.0
    char_height = (int)(ascender - descender)
 
    # 创建一张图片来保存文字
    image = Image.new('L', (img_width, img_height))
    
    # 获取图片的像素数据
    pixels = image.load()
    
    pos_x, pos_y = 10, 10
    for cur_char in text:
        ft_face.load_char(cur_char)
        if cur_char == ' ':
            advance_width = char_height // 4
            if advance_width == 0:
                advance_width = 1
            width = advance_width
            height = 1
            offset_x = 0
            offset_y = 0

            pos_x += advance_width
        else:
            slot = ft_face.glyph
            
            # 获取渲染的宽度和高度
            advance_width = slot.advance.x >> 6
            width = slot.bitmap.width
            height = slot.bitmap.rows
            bearing_x = slot.bitmap_left
            bearing_y = slot.bitmap_top

            offset_x = bearing_x
            offset_y = ascender - bearing_y
            char_pos_x = pos_x + offset_x
            char_pos_y = pos_y + offset_y

            # 将FreeType的位图数据复制到PIL图片
            for y in range(height):
                for x in range(width):
                    pixels[char_pos_x + x, char_pos_y + y] = slot.bitmap.buffer[y * width + x]
 
            # 更新文本宽度和y坐标
            pos_x += advance_width
 
    # 保存图片
    image.save(output_filename)
 
# 使用方法
test_text = "Hello World!"
font_filename = "build_in/Montserrat-Medium.ttf"
render_text(test_text, font_filename, 16)