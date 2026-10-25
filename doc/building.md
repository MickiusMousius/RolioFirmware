# Rolio Firmware Build Guide

> ⚠️ **NOTE:** Build commands must be executed from the terminal _inside_ the VS Code container.

## Clone the Rolio Firmware Repository

You can either clone the repository manually (ensuring it is placed in the `zmk-modules` folder) or you can clone it directly from within your container's CLI using the following commands:

```bash
cd /workspaces/zmk-modules
git clone git@github.com:MickiusMousius/RolioFirmware.git

```

> **Tip:** To work on a branch other than `main`, add `-b <branch>` to the `git clone` command (e.g., `git clone -b dev git@github.com:MickiusMousius/RolioFirmware.git`).

## Retrieve Rolio Dependencies

This repository contains a simple Python tool for pulling down the list of dependencies defined in the `west.yml` file. You can use it to get all of the Rolio dependencies with the following command:

```bash
/workspaces/zmk-modules/RolioFirmware/tools/clone_west_repos.py -m /workspaces/zmk-modules/RolioFirmware/config/west.yml -d /workspaces/zmk-modules

```

> **Tip:** This exact same command can be used later to update your cloned dependencies.

## Build the Firmware (Automated)

There is a small wrapper script included to simplify building your firmware. It can be invoked from the CLI in your container as follows:

```bash
/workspaces/zmk-modules/RolioFirmware/tools/build_rolio.sh

```

When you build the firmware using this script, you will find the final compiled files in the cloned Rolio repository under `build/zmk/`, named after the board & display they were built for (e.g., `rolio_nice_nano_v2-vista508_left.uf2`).

The build wrapper accepts the following options, which can be combined in any order:

| Option | Description |
| --- | --- |
| `left` / `right` / `all` | Build just the left side, just the right side, or both (the default). |
| `--board nice_nano_v2` / `--board mikoto@7.2.0` | The controller to build for (defaults to `nice_nano_v2`). |
| `--display vista508` / `--display nice_view` | The display to build for (defaults to `vista508`). |
| `usb_logging` | Include the `zmk-usb-logging` snippet so that logs can be read over USB. |

For example, to build only the left side for a Mikoto with a nice!view display:

```bash
/workspaces/zmk-modules/RolioFirmware/tools/build_rolio.sh left --board mikoto@7.2.0 --display nice_view

```

## Build the Firmware (Manual)

You may encounter issues if you are doing more advanced local development or if the wrapper script fails to maintain the correct module paths. In that case, you will want to build the firmware manually.

First, enter the ZMK app directory in your dev container:

```bash
cd /workspaces/zmk/app

```

**Build the left side firmware:**

```bash
west build -d /workspaces/zmk-modules/RolioFirmware/build/left -p -b "nice_nano_v2" -S studio-rpc-usb-uart -- -DZMK_EXTRA_MODULES="/workspaces/zmk-modules/RolioFirmware;/workspaces/zmk-modules/zmk-ls0xxvcom-driver;/workspaces/zmk-modules/zmk-defaultlayer;/workspaces/zmk-modules/zmk-patch-batterylevel;/workspaces/zmk-modules/zmk-patch-ec11;/workspaces/zmk-modules/zmk-smart-sleep;/workspaces/zmk-modules/zmk-ble-passkey" -DZMK_CONFIG="/workspaces/zmk-modules/RolioFirmware/config" -DSHIELD="rolio_left vista508"

```

**Build the right side firmware:**

```bash
west build -d /workspaces/zmk-modules/RolioFirmware/build/right -p -b "nice_nano_v2" -- -DZMK_EXTRA_MODULES="/workspaces/zmk-modules/RolioFirmware;/workspaces/zmk-modules/zmk-ls0xxvcom-driver;/workspaces/zmk-modules/zmk-defaultlayer;/workspaces/zmk-modules/zmk-patch-batterylevel;/workspaces/zmk-modules/zmk-patch-ec11;/workspaces/zmk-modules/zmk-smart-sleep;/workspaces/zmk-modules/zmk-ble-passkey" -DZMK_CONFIG="/workspaces/zmk-modules/RolioFirmware/config" -DSHIELD="rolio_right vista508"

```

When building manually, you will find the compiled firmware in the cloned Rolio repository under `build/left/zephyr/zmk.uf2` and `build/right/zephyr/zmk.uf2`.

## Development Tools

The `tools` folder also contains a few utilities to help when making changes to the firmware.

**Check the build script & `west.yml` agree on which modules are used:**

```bash
/workspaces/zmk-modules/RolioFirmware/tools/check_modules.py -b /workspaces/zmk-modules/RolioFirmware/tools/build_rolio.sh -w /workspaces/zmk-modules/RolioFirmware/config/west.yml

```

**Clean up the code formatting:**

```bash
/workspaces/zmk-modules/RolioFirmware/tools/clean_code.sh

```

This runs `clang-format` over the C source (skipping generated image data), spaces out top level functions & redraws the box diagrams around each layer in `rolio.keymap`. It's a good idea to run it, then rebuild, before committing changes.

**Regenerate the display images & animations:**

The Vista508 display's icons & animations are generated from the GIFs in `boards/shields/vista508/src/assets/images/raw` & `boards/shields/vista508/src/assets/animations/raw`. After editing a GIF, regenerate its C file with the script alongside it:

```bash
/workspaces/zmk-modules/RolioFirmware/boards/shields/vista508/src/assets/images/raw/update_c_files.sh
/workspaces/zmk-modules/RolioFirmware/boards/shields/vista508/src/assets/animations/generate_animations.sh

```

The art shown on the right side's display is generated separately, see the [image converter](../image_converter/readme.md).
