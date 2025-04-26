# Introduction
This directory contains small Python script that can be used to customize the artwork displayed on the peripheral side of a Rolio46.

It works by taking a folder of images and converting them into a single "art.c" file.

If you fork this Git repository you can replace the existing art.c with the new one and push the changes to your fork.

Once you've pushed the changes to your fork GitHub actions should automatically generate a new firmware image for you.

You should not use too many images as your keyboard may not work or the firmware may not build. I've tested with 90 images successfully, however I'd strongly recommend aiming for 25 or less so that you have plenty of safety margin.

# Usage Instructions

From the "image_converter" directory run the following (using a bash or zsh shell):

    ./setup.sh
    source venv/bin/activate

The above will setup a virtual environment for you, you can now use the tool to generate a new art.c file. I have included some sample art in the repo, to use it you would do the following:

    python3 converter/main.py --inDir sample_images/ai_art --outDir outputs

This will generate a new art.c file and previews of what the converted results will look like in the "outputs" folder.

The converted images in the "previews" folder can be used as inputs too.

To use the generated "art.c" file copy it into the "boards/vista508/widgets" folder of the firmware repository, commit the change and push it to GitHub. A GitHub action should then rebuild your firmware with your newly selected images.

Some tips to help with converting your art:
 * Use grey scale art that has strong contrast
 * Fine lines will not render nicely so using bold lines is a good idea
 * Experiment with different scaling algorithms and thresholds
 * Check the previews sub directory in your output folder before pushing changes to GitHub
 * Run the following to get help:
       
       >> python3 converter/main.py --help

 * If you find that you need to use a few different sets of options you can take the images from the outputs/previews folder that you like and reuse them as inputs.
 * The scaled images are 144x147 pixels, keep your art to a similar aspect ratio if you can, the tool will rescale for you where it can.
