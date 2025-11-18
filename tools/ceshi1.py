from PIL import Image
import struct

def image_to_rgb565_bin(input_path, output_path, width=240, height=320):
    """
    Converts an image to the standard RGB565 format.
    - Red:   5 bits (Bits 15-11)
    - Green: 6 bits (Bits 10-5)
    - Blue:  5 bits (Bits 4-0)
    This is the standard format for most TFT screens.
    """
    try:
        img = Image.open(input_path).convert("RGB")
        if img.size != (width, height):
            print(f"Info: Resizing image from {img.size} to {width}x{height}.")
            img = img.resize((width, height))

        with open(output_path, 'wb') as f:
            for y in range(height):
                for x in range(width):
                    r, g, b = img.getpixel((x, y))

                    # 8-bit R,G,B -> 5-bit R, 6-bit G, 5-bit B
                    r_5 = r >> 3
                    g_6 = g >> 2
                    b_5 = b >> 3
                    
                    # 关键改动在这里！
                    # 按照 RRRRR GGGGGG BBBBB 的顺序组合成16位数据
                    pixel_color = (r_5 << 11) | (g_6 << 5) | b_5
                        
                    # 写入文件，使用大端字节序 '>H'
                    f.write(struct.pack('>H', pixel_color))

        print(f"成功! 已生成标准的 RGB565 格式文件: {output_path}")

    except Exception as e:
        print(f"发生错误: {e}")

# --- 使用方法 ---
input_image_file = "input_image.png"  # 换成你的图片
output_bin_file = "data.txt"
image_to_rgb565_bin(input_image_file, output_bin_file)
