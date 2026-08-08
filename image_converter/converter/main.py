import argparse
import pathlib
import sys

from PIL import Image, ImageOps, UnidentifiedImageError

CODE_PREFIX = """
// From Image: %s
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t
    %s_map[] = {
#if CONFIG_NICE_VIEW_WIDGET_INVERTED
        0xff, 0xff, 0xff, 0xff, /*Color of index 0*/
        0x00, 0x00, 0x00, 0xff, /*Color of index 1*/
#else
        0x00, 0x00, 0x00, 0xff, /*Color of index 0*/
        0xff, 0xff, 0xff, 0xff, /*Color of index 1*/
#endif
"""

CODE_SUFFIX = """
};
const lv_img_dsc_t %s = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = %d,
    .header.h = %d,
    .data_size = %d,
    .data = %s_map,
};

"""

ART_C_BODY = """
#include <lvgl.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

%s

const lv_img_dsc_t image_list[%d] = {
    %s
};

const uint32_t image_count = %d;

"""


def image_to_black_and_white(source_image, threshold=None):
    if threshold:
        return source_image.convert("L").point(
            lambda x: 255 if x > threshold else 0, mode="1"
        )
    return source_image.convert("1")


def image_to_byte_array(source_image):
    byte_values = [128, 64, 32, 16, 8, 4, 2, 1]
    image_byte_array = []

    # Extract all pixels sequentially
    pixels = list(source_image.getdata())

    # Process the pixels in chunks of 8
    for i in range(0, len(pixels), 8):
        chunk = pixels[i : i + 8]
        # Ensure the chunk has 8 elements (pad with 0 if necessary)
        if len(chunk) < 8:
            chunk.extend([0] * (8 - len(chunk)))

        pixels_byte = sum(byte_values[j] for j in range(8) if not chunk[j])
        image_byte_array.append(pixels_byte)

    return image_byte_array


def byte_array_to_hex_code(byte_array):
    # Convert bytes to hex strings
    hex_strings = [f"0x{b:02x}," for b in byte_array]
    lines = []

    # 144 pixels, break onto a new line of C (just like the target image)
    for i in range(0, len(hex_strings), 18):
        lines.append("        " + " ".join(hex_strings[i : i + 18]))

    return "\n".join(lines)


def image_to_c_code(source_image, image_name, file_name):
    image_byte_array = image_to_byte_array(source_image)
    hex_code = byte_array_to_hex_code(image_byte_array)
    magic_bytes = int(42 + (source_image.width * source_image.height / 8))

    # C code templates heavily use {}, so we keep the % operator for formatting
    output_code = CODE_PREFIX % (file_name, image_name)
    output_code += hex_code
    output_code += CODE_SUFFIX % (
        image_name,
        source_image.width,
        source_image.height,
        magic_bytes,
        image_name,
    )
    return output_code


def scale_and_crop_image(source_image, black_background, algorithm=None):
    if source_image.mode == "L":
        background = 0 if black_background else 255
    else:
        background = (0, 0, 0) if black_background else (255, 255, 255)

    # Choose our scaling algorithm
    algo = Image.Resampling.NEAREST
    if algorithm:
        algo = getattr(Image.Resampling, algorithm)

    # Rescale the image to fit on our display
    temp_image = source_image.copy()
    temp_image.thumbnail((144, 147), algo)

    # Add padding to fill the whole screen and ensure things are centered
    if temp_image.width < 144 or temp_image.height < 144:
        x_pos = (144 - temp_image.width) // 2
        y_pos = (147 - temp_image.height) // 2
        result = Image.new(temp_image.mode, (144, 147), background)
        result.paste(temp_image, (x_pos, y_pos))
        return result

    return temp_image


def main():
    parser = argparse.ArgumentParser(
        description="Convert a set of images to an art.c file for the Vista508",
        allow_abbrev=True,
    )
    parser.add_argument(
        "--inDir",
        type=pathlib.Path,
        required=True,
        dest="in_dir",
        help="Directory containing the input images",
    )
    parser.add_argument(
        "--outDir",
        type=pathlib.Path,
        required=True,
        dest="out_dir",
        help='Directory to output the sample images and final "art.c" file to',
    )
    parser.add_argument(
        "--threshold",
        type=int,
        required=False,
        choices=range(256),
        metavar="[0-255]",
        help=(
            "Black level threshold for image conversion, if not specified "
            "conversion will use dithering to convert greys"
        ),
    )
    parser.add_argument(
        "--scalingAlgorithm",
        choices=["NEAREST", "BOX", "BILINEAR", "HAMMING", "BICUBIC", "LANCZOS"],
        type=str,
        required=False,
        dest="scaling_algorithm",
        help="Algorithm to use when shrinking images to fit on the Vista508",
    )
    parser.add_argument(
        "--invert",
        required=False,
        default=False,
        action="store_true",
        help="Make a negative image",
    )
    parser.add_argument(
        "--blackBackground",
        required=False,
        default=False,
        action="store_true",
        dest="black_background",
        help="Use black pixels to fill in the empty canvas",
    )
    args = parser.parse_args()

    # Validate our input paths
    if not args.in_dir.is_dir():
        sys.exit(f"Invalid input directory path: {args.in_dir}")
    if not args.out_dir.is_dir():
        sys.exit(f"Invalid output directory path: {args.out_dir}")

    image_counter = 0
    art_c = ""
    image_list = []

    # Process images
    for full_path in args.in_dir.iterdir():
        if full_path.is_file():
            print(f"Converting image: {full_path}")
            try:
                img = Image.open(full_path)
            except UnidentifiedImageError:
                print("    Image invalid, skipping...")
                continue

            if args.invert:
                img = ImageOps.invert(img)

            temp_image = scale_and_crop_image(
                img, args.black_background, algorithm=args.scaling_algorithm
            )
            bw_image = image_to_black_and_white(temp_image, threshold=args.threshold)

            # Write our temporary black and white image to our preview folder
            preview_dir = args.out_dir / "previews"
            preview_dir.mkdir(parents=True, exist_ok=True)

            preview_path = preview_dir / full_path.name
            print(f"    Saving preview to: {preview_path}")
            bw_image.save(preview_path)

            image_name = f"image{image_counter}"
            art_c += image_to_c_code(bw_image, image_name, full_path.name)
            image_list.append(image_name)
            image_counter += 1

    complete_art_c = ART_C_BODY % (
        art_c,
        image_counter,
        ", ".join(image_list),
        image_counter,
    )

    # Write out the new art.c file
    art_path = args.out_dir / "art.c"
    print(f"Writing art file to: {art_path}")
    with open(art_path, "w") as art_out_file:
        art_out_file.write(complete_art_c)


if __name__ == "__main__":
    main()
