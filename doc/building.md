# Rolio Firmware Build Guide

> ⚠️ **NOTE:** Build commands must be executed from the terminal _inside_ the VS Code container.

## Clone the Rolio Firmware Repository

You can either clone the repository manually (ensuring it is placed in the `zmk-modules` folder) or you can clone it directly from within your container's CLI using the following commands:

```bash
cd /workspaces/zmk-modules
git clone git@github.com:MickiusMousius/RolioFirmware.git

```

## Retrieve Rolio Dependencies

This repository contains a simple Python tool for pulling down the list of dependencies defined in the `west.yml` file. You can use it to get all of the Rolio dependencies with the following command:

```bash
/workspaces/zmk-modules/RolioFirmware/tools/clone_west_repos.py -m /workspaces/zmk-modules/RolioFirmware/config/west.yml -d /workspaces/zmk-modules

```

> **Tip:** This exact same command can be used later to update your cloned dependencies.

## Build the Firmware (Automated)

There is a small wrapper script included to simplify building your firmware. It can be invoked from the CLI in your container as follows:

```bash
/workspaces/zmk-modules/rolio/tools/build_rolio.sh

```

When you build the firmware using this script, you will find the final compiled files in the cloned Rolio repository under `build/zmk/`.

If you wish to build just the left or right side, you may pass a `left` or `right` flag to the build wrapper (e.g., `./build_rolio.sh left`).

## Build the Firmware (Manual)

You may encounter issues if you are doing more advanced local development or if the wrapper script fails to maintain the correct module paths. In that case, you will want to build the firmware manually.

First, enter the ZMK app directory in your dev container:

```bash
cd /workspaces/zmk/app

```

**Build the left side firmware:**

```bash
west build -d /workspaces/zmk-modules/rolio/build/left -p -b "nice_nano_v2" -S studio-rpc-usb-uart -- -DZMK_EXTRA_MODULES="/workspaces/zmk-modules/zmk-userspace/;/workspaces/zmk-modules/RolioFirmware/;/workspaces/zmk-modules/zmk-ls0xxvcom-driver;/workspaces/zmk-modules/zmk-patch-batterylevel;/workspaces/zmk-modules/zmk-patch-ec11" -DZMK_CONFIG="/workspaces/zmk-modules/RolioFirmware/config" -DSHIELD="rolio_left vista508"

```

**Build the right side firmware:**

```bash
west build -d /workspaces/zmk-modules/rolio/build/left -p -b "nice_nano_v2" -S studio-rpc-usb-uart -- -DZMK_EXTRA_MODULES="/workspaces/zmk-modules/zmk-userspace/;/workspaces/zmk-modules/RolioFirmware/;/workspaces/zmk-modules/zmk-ls0xxvcom-driver;/workspaces/zmk-modules/zmk-patch-batterylevel;/workspaces/zmk-modules/zmk-patch-ec11" -DZMK_CONFIG="/workspaces/zmk-modules/RolioFirmware/config" -DSHIELD="rolio_right vista508"

```

When building manually, you will find the compiled firmware in the cloned Rolio repository under `build/left/zephyr/zmk.uf2` and `build/right/zephyr/zmk.uf2`.
