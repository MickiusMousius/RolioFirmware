import argparse
from PIL import Image

def convert_gif_to_lvgl_c(gif_path, c_path, threshold=64, prefix="dance", generate_inv=False):
    img = Image.open(gif_path)
    frames = []
    
    try:
        while True:
            # Convert frame to L (grayscale)
            frame = img.copy().convert('L')
            frames.append(frame)
            img.seek(img.tell() + 1)
    except EOFError:
        pass
        
    # Drop frames to convert 30FPS to 10FPS (keep every 3rd frame)
    frames = frames[::3]
        
    width, height = frames[0].size
    
    with open(c_path, 'w') as f:
        f.write("#include <lvgl.h>\n\n")
        
        for i, frame in enumerate(frames):
            pixels = list(frame.getdata())
            byte_array = []
            
            row_bytes = (width + 7) // 8
            
            for y in range(height):
                for byte_x in range(row_bytes):
                    b = 0
                    for bit in range(8):
                        x = byte_x * 8 + bit
                        if x < width:
                            pixel = pixels[y * width + x]
                            # Inverted colors: <= threshold = black (index 1), > threshold = white (index 0)
                            if pixel <= threshold:
                                b |= (1 << (7 - bit))
                    byte_array.append(b)
                    
            f.write(f"const uint8_t {prefix}_frame_{i}_map[] = {{\n")
            f.write("    0xff, 0xff, 0xff, 0xff, /*Color of index 0*/\n")
            f.write("    0x00, 0x00, 0x00, 0xff, /*Color of index 1*/\n")
            f.write("    ")
            for j, b in enumerate(byte_array):
                f.write(f"0x{b:02x}, ")
                if (j + 1) % 12 == 0:
                    f.write("\n    ")
            f.write("\n};\n\n")

            if generate_inv:
                f.write(f"const uint8_t {prefix}_frame_{i}_map_inv[] = {{\n")
                f.write("    0x00, 0x00, 0x00, 0xff, /*Color of index 0*/\n")
                f.write("    0xff, 0xff, 0xff, 0xff, /*Color of index 1*/\n")
                f.write("    ")
                for j, b in enumerate(byte_array):
                    f.write(f"0x{b:02x}, ")
                    if (j + 1) % 12 == 0:
                        f.write("\n    ")
                f.write("\n};\n\n")
            
            f.write(f"const lv_img_dsc_t {prefix}_frame_{i} = {{\n")
            f.write("    .header.always_zero = 0,\n")
            f.write("    .header.w = " + str(width) + ",\n")
            f.write("    .header.h = " + str(height) + ",\n")
            f.write("    .data_size = " + str(len(byte_array) + 8) + ",\n")
            f.write("    .header.cf = LV_IMG_CF_INDEXED_1BIT,\n")
            f.write(f"    .data = {prefix}_frame_{i}_map,\n")
            f.write("};\n\n")

            if generate_inv:
                f.write(f"const lv_img_dsc_t {prefix}_frame_{i}_inv = {{\n")
                f.write("    .header.always_zero = 0,\n")
                f.write("    .header.w = " + str(width) + ",\n")
                f.write("    .header.h = " + str(height) + ",\n")
                f.write("    .data_size = " + str(len(byte_array) + 8) + ",\n")
                f.write("    .header.cf = LV_IMG_CF_INDEXED_1BIT,\n")
                f.write(f"    .data = {prefix}_frame_{i}_map_inv,\n")
                f.write("};\n\n")
            
        f.write(f"const lv_img_dsc_t *{prefix}_frames[] = {{\n")
        for i in range(len(frames)):
            f.write(f"    &{prefix}_frame_{i},\n")
        f.write("};\n\n")

        if generate_inv:
            f.write(f"const lv_img_dsc_t *{prefix}_frames_inv[] = {{\n")
            for i in range(len(frames)):
                f.write(f"    &{prefix}_frame_{i}_inv,\n")
            f.write("};\n\n")

        f.write(f"const uint32_t {prefix}_frame_count = {len(frames)};\n")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert GIF to LVGL 1-bit indexed C array.')
    parser.add_argument('gif_path', type=str, help='Path to the input GIF file')
    parser.add_argument('c_path', type=str, help='Path to the output C file')
    parser.add_argument('--threshold', type=int, default=64, help='Grayscale threshold (0-255). Above this is black.')
    parser.add_argument('--prefix', type=str, default='dance', help='Prefix for generated C variables')
    parser.add_argument('--dual', action='store_true', help='Generate inverted image structures alongside standard ones')
    args = parser.parse_args()
    
    convert_gif_to_lvgl_c(args.gif_path, args.c_path, args.threshold, args.prefix, args.dual)
