from PIL import Image
import struct
import os

def rgb565_bin_to_image(input_path, output_path, width=240, height=320):
    """
    将 RGB565 格式的原始二进制文件还原为图片。
    """
    try:
        # 1. 检查文件大小是否匹配预期的尺寸
        expected_size = width * height * 2
        file_size = os.path.getsize(input_path)
        if file_size != expected_size:
            print(f"错误：文件大小不匹配预期的 {width}x{height} RGB565 尺寸。")
            print(f"预期大小: {expected_size} 字节，实际大小: {file_size} 字节。")
            return

        # 2. 准备新的图片对象和像素数据列表
        img = Image.new("RGB", (width, height))
        pixels = []

        # 3. 读取二进制文件
        with open(input_path, 'rb') as f:
            # 读取所有数据，每个像素 2 字节
            raw_data = f.read()

        # 4. 遍历数据并进行 RGB565 -> RGB888 转换
        # 数据是小端序 ('<H')，所以使用 2 字节步长
        for i in range(0, len(raw_data), 2):
            # 解包 16 位小端序整数
            # struct.unpack 返回一个元组，取第一个元素
            rgb565 = struct.unpack('<H', raw_data[i:i+2])[0]

            # 提取 R5 G6 B5
            r5 = (rgb565 >> 11) & 0x1F  # R 是高 5 位
            g6 = (rgb565 >> 5) & 0x3F   # G 是中间 6 位
            b5 = rgb565 & 0x1F          # B 是低 5 位

            # R5 G6 B5 转换回 R8 G8 B8
            # 5位转8位：R8 = (R5 << 3) | (R5 >> 2)
            # 6位转8位：G8 = (G6 << 2) | (G6 >> 4)
            r = (r5 << 3) | (r5 >> 2)
            g = (g6 << 2) | (g6 >> 4)
            b = (b5 << 3) | (b5 >> 2)

            # 将 (R, G, B) 元组添加到像素列表
            pixels.append((r, g, b))

        # 5. 将像素数据填充到图片对象中
        img.putdata(pixels)

        # 6. 保存图片
        img.save(output_path)
        print(f"成功将二进制数据还原为图片，保存至: {output_path}")

    except FileNotFoundError:
        print(f"错误：未找到文件 {input_path}")
    except Exception as e:
        print(f"处理文件时发生错误: {e}")

# --- 使用示例 ---
input_bin_file = "data.txt"  # 您的 RGB565 二进制文件
output_image_file = "restored_image.png" # 还原后的图片文件名

# 确保尺寸与生成脚本中的一致
IMAGE_WIDTH = 240
IMAGE_HEIGHT = 320

rgb565_bin_to_image(input_bin_file, output_image_file, IMAGE_WIDTH, IMAGE_HEIGHT)
