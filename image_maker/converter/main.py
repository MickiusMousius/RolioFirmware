from PIL import Image, ImageOps, UnidentifiedImageError
import sys
import argparse
import pathlib
import os


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


def ImageToBlackAndWhite(sourceImage, threshold=None):
    if threshold:
        fn = lambda x: 255 if x > threshold else 0
        return sourceImage.convert('L').point(fn, mode='1')
    return sourceImage.convert('1')


def ImageToByteArray(sourceImage):
    BYTE_VALUES = [128, 64, 32, 16, 8, 4, 2, 1]
    imageByteArray = []
    currentByte = []
    for y in range(0, sourceImage.height):
        for x in range(0, sourceImage.width):
            if len(currentByte) == 8:
                pixelsByte = sum(BYTE_VALUES[i] for i in range(0, 8)
                                 if not currentByte[i])
                imageByteArray.append(pixelsByte)
                currentByte = []
            pixel = sourceImage.getpixel((x, y))
            currentByte.append(pixel)
    pixelsByte = sum(BYTE_VALUES[i] for i in range(0, 8) if not currentByte[i])
    imageByteArray.append(pixelsByte)
    return imageByteArray


def ByteArrayToHexCode(byteArray):
    # convert to image C code
    colCounter = 0
    rowAccumulator = "        "
    for thisByte in byteArray:
        hexString = "0x%02x, " % thisByte
        # 144 pixels, break onto a new line of C (just like teh target image)
        if colCounter == 18:
            rowAccumulator = rowAccumulator + "\n        "
            colCounter = 0
        rowAccumulator = rowAccumulator + hexString
        colCounter += 1
    return rowAccumulator


def ImageToCCode(sourceImage, imageName, fileName):
    imageByteArray = ImageToByteArray(sourceImage)
    hexCode = ByteArrayToHexCode(imageByteArray)
    magicBytes = 42 + (sourceImage.width * sourceImage.height / 8)
    outputCode = CODE_PREFIX % (fileName, imageName)
    outputCode += hexCode
    outputCode += CODE_SUFFIX % (imageName,
                                 sourceImage.width,
                                 sourceImage.height,
                                 magicBytes,
                                 imageName)
    return outputCode


def ScaleAndCropImage(sourceImage, blackBackground, algorithm=None):
    if sourceImage.mode == 'L':
        background = 255
        if blackBackground:
            background = 0
    else:
        background = (255, 255, 255)
        if blackBackground:
            background = (0, 0, 0)
    # Choose our scalling algoirthm
    algo = Image.Resampling.NEAREST
    if algorithm:
        algo = getattr(Image.Resampling, algorithm)
    # Rescale the image to fit on our display
    tempImage = sourceImage.copy()
    tempImage.thumbnail((144, 147), algo)
    # Add padding to fill the whhole screen and ensure things are centered
    if tempImage.width < 144 or tempImage.height < 144:
        xPos = (144 - tempImage.width) // 2
        yPos = (147 - tempImage.height) // 2
        result = Image.new(tempImage.mode, (144, 147), background)
        result.paste(tempImage, (xPos, yPos))
        return result
    return tempImage


parser = argparse.ArgumentParser(
    description='Convert a set of images to an art.c file for the Vista508',
    allow_abbrev=True)
parser.add_argument(
    '--inDir',
    type=pathlib.Path,
    required=True,
    dest='inDir',
    help='Directory containing te input images')
parser.add_argument(
    '--outDir',
    type=pathlib.Path,
    required=True,
    dest='outDir',
    help='Directory to ouput the sample images and final "art.c" file to')
parser.add_argument(
    '--threshold',
    type=int,
    required=False,
    choices=range(0,255),
    metavar="[0-255]",
    help="".join(
        ["Black level threshold for image conversion, ",
         "if not specified coversion will use dithering to convert greys"
         ]))
parser.add_argument(
    '--scalingAlgorithm',
    choices=["NEAREST", "BOX", "BILINEAR", "HAMMING", "BICUBIC", "LANCZOS"],
    type=str,
    required=False,
    help='Algorithm to use when shrinking images to fit on the Vista508')
parser.add_argument(
    '--invert',
    required=False,
    default=False,
    action='count',
    help='Make a negative image')
parser.add_argument(
    '--blackBackground',
    required=False,
    default=False,
    action='count',
    help='Use black pixels to fill in the empty canvas')
args = parser.parse_args()

# Validate our input paths
if not os.path.isdir(args.inDir):
    print("Invalid inoput directory path: %s" % (args.inDir))
    exit(1)
if not os.path.isdir(args.outDir):
    print("Invalid output directory path: %s" % (args.outDir))
    exit(1)

imageCounter = 0
artC = ""
imageList = ""
for fName in os.listdir(args.inDir):
    fullPath = os.path.join(args.inDir, fName)
    if os.path.isfile(fullPath):
        print("Converting image: %s" % (fullPath))
        try:
            img = Image.open(fullPath)
        except UnidentifiedImageError:
            print("    Image invalid skipping...")
            continue
        if args.invert:
            img = ImageOps.invert(img)
        tempImage = ScaleAndCropImage(img,
                                      args.blackBackground,
                                      algorithm=args.scalingAlgorithm)
        bwImage = ImageToBlackAndWhite(tempImage, threshold=args.threshold)
        # Write our temporay black and white image to our preview folder
        previewDir = os.path.join(args.outDir, "previews")
        if not os.path.exists(previewDir):
            os.makedirs(previewDir)
        previewPath = os.path.join(previewDir, fName)
        print("    Saving preview to: %s" % previewPath)
        bwImage.save(previewPath)
        imageName = 'image%d' % imageCounter
        artC += ImageToCCode(bwImage, imageName, fName)
        imageList += imageName + ", "
        imageCounter += 1

completeArtC = ART_C_BODY % (artC, imageCounter, imageList, imageCounter)

# Write out the new art.c file
artPath = os.path.join(args.outDir, "art.c")
print("Writing art file to: %s" % artPath)
with open(artPath, 'w') as artOutFile:
    artOutFile.write(completeArtC)

exit()
