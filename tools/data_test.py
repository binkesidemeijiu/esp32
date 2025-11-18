from PIL import Image
import struct
import os

def image_to_rgb565_bin(input_path, output_path):
    try:
        # 1. 打开图片并转换为 RGB 模式
        img = Image.open(input_path).convert("RGB")
        width, height = img.size
        print(f"原始图片尺寸: {width}x{height}")

        # 检查尺寸是否符合目标要求 (320x240)
        if width != 240 or height != 320:
            print("WARN: 尺寸不匹配。将图片缩放/裁剪至 320x240。")
            # 这里简单使用 resize，您也可以根据需求使用 crop
            img = img.resize((240,320))
            width, height = img.size

        # 2. 准备输出文件
        with open(output_path, 'wb') as f:
            # 3. 遍历每个像素并转换为 RGB565
            for y in range(height):
                for x in range(width):
                    # 获取 RGB 颜色值
                    r, g, b = img.getpixel((x, y))
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
                    f.write(struct.pack('<H', rgb565))

        print(f"成功将图片转换为 {width}x{height} RGB565 格式，保存至: {output_path}")
        print(f"文件大小: {os.path.getsize(output_path)} 字节")
        
    except FileNotFoundError:
        print(f"错误：未找到文件 {input_path}")
    except Exception as e:
        print(f"处理图片时发生错误: {e}")

# --- 使用示例 ---
input_image_file = "input_image.png"  # 替换为您的图片文件路径
output_bin_file = "data.txt"          # 替换为您希望存储在 SD 卡上的文件名

image_to_rgb565_bin(input_image_file, output_bin_file)