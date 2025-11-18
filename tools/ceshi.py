from PIL import Image
import struct

def image_to_bgr565_bin(input_path, output_path, width=240, height=320):
    """
    Converts an image to BGR565 format for ST7789V displays
    configured with MADCTL=0x08 (BGR color order).
    - Blue:  5 bits
    - Green: 6 bits
    - Red:   5 bits
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
                    
                    # Assemble into a 16-bit word in BBBBB GGGGGG RRRRR order
                    pixel_color = (b_5 << 11) | (g_6 << 5) | r_5
                        
                    # Write to file using BIG-ENDIAN byte order '>H'
                    # TFT controllers expect the high byte first
                    f.write(struct.pack('>H', pixel_color))

        print(f"成功! 已生成匹配硬件的 BGR565 格式文件: {output_path}")

    except Exception as e:
        print(f"发生错误: {e}")

# --- 使用方法 ---
input_image_file = "input_image.png"  # 换成你的图片
output_bin_file = "data.txt"
image_to_bgr565_bin(input_image_file, output_bin_file)
