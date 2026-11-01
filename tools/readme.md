# RolioFirmware Tools

This folder contains the scripts used to fetch dependencies, build, format & document the Rolio46 firmware.

> ⚠️ **NOTE:** These tools are intended to be run from the terminal _inside_ the ZMK v0.3 dev container described in the [development environment guide](../doc/tools_setup.md). Several of them assume the container's folder layout (`/workspaces/zmk`, `/workspaces/zmk-modules/...`).

| Tool | Purpose |
| --- | --- |
| [`clone_west_repos.py`](#clone_west_repospy) | Clones or updates the ZMK modules listed in `config/west.yml`. |
| [`build_rolio.sh`](#build_roliosh) | Builds the left and/or right firmware for a given controller & display. |
| [`check_modules.py`](#check_modulespy) | Checks that `build_rolio.sh` & `west.yml` agree on which modules are used. |
| [`clean_code.sh`](#clean_codesh) | Runs all of the formatting tools below in one go. |
| [`add_blank_lines.py`](#add_blank_linespy) | Spaces out top level functions & structs in the C source. |
| [`format.sh`](#formatsh) | Runs `clang-format` over the C source. |
| [`format_keymap.py`](#format_keymappy) | Redraws the box diagrams around each layer in `rolio.keymap`. |
| [`draw_keymap.sh`](#draw_keymapsh--draw_keymappy) / [`draw_keymap.py`](#draw_keymapsh--draw_keymappy) | Draws the keymap diagram (`doc/images/keymap.svg`) shown in the main README. |
| [`gif_to_lvgl.py`](#gif_to_lvglpy) | Converts a GIF into 1-bit LVGL image data for the Vista508 display. |

<br/>

# Fetching Dependencies & Building

See the [Building Guide](../doc/building.md) for the full walk-through, the tools it uses are summarised below.

## `clone_west_repos.py`

Clones (or updates) every project listed in a `west.yml` manifest into a destination folder. This is how the Rolio's ZMK modules end up alongside `RolioFirmware` in `/workspaces/zmk-modules`, without needing a full `west` workspace for them.

```bash
/workspaces/zmk-modules/RolioFirmware/tools/clone_west_repos.py -m /workspaces/zmk-modules/RolioFirmware/config/west.yml -d /workspaces/zmk-modules
```

| Option | Description |
| --- | --- |
| `-m`, `--manifest` | Path to the `west.yml` file (required). |
| `-d`, `--dest` | Folder to clone the repositories into (required). |
| `-f`, `--force` | Discard local changes & reset diverged branches without asking. |

How it behaves:

- The `zmk` project is skipped, ZMK itself is the dev container's own checkout.
- Repository URLs are built from the manifest's `remotes` (or a project's explicit `url`) & each project is checked out at its `revision`.
- Re-running the same command updates existing clones. Branches are fast-forwarded, while a pinned commit or tag is simply checked out.
- If a clone has uncommitted changes, or its branch has diverged from `origin`, you'll be asked whether to **s**kip it, **f**orce overwrite it or **a**bort.

> ⚠️ **NOTE:** Forcing (either with `-f` or by answering `f`) runs `git reset --hard` & `git clean -fd`, any uncommitted work in that module will be lost.

## `build_rolio.sh`

A wrapper around `west build` that builds the firmware with all of the Rolio's modules & copies the resulting UF2 files somewhere easy to find.

```bash
/workspaces/zmk-modules/RolioFirmware/tools/build_rolio.sh
```

The options can be combined in any order:

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

Things worth knowing:

- Finished firmware is copied to `build/zmk/` & named `rolio_<board>-<display>_<side>.uf2` (e.g. `rolio_nice_nano_v2-vista508_left.uf2`). The board's `@version` is left out of the name.
- The intermediate build folders live in `build/zmk/<board>-<display>/left` & `.../right`. Every build is a pristine (`-p`) build.
- The left side (the central) is built with the `studio-rpc-usb-uart` snippet so that it works with [ZMK Studio](https://zmk.studio).
- Building for the nice!view adds `config/rolio-nice_view.conf`, which switches to ZMK's built in status screen.
- The list of modules passed to `ZMK_EXTRA_MODULES` lives in the `SHARED_MODULES`, `LEFT_ONLY_MODULES` & `RIGHT_ONLY_MODULES` arrays. If you add a module to `west.yml`, add it here too (then run [`check_modules.py`](#check_modulespy)).
- The script stops with an error if a side fails to produce a UF2.

## `check_modules.py`

Cross-references the modules used by `build_rolio.sh` with the projects in `west.yml`, so it's easy to spot a module that's downloaded but never built, or built but never downloaded.

```bash
/workspaces/zmk-modules/RolioFirmware/tools/check_modules.py -b /workspaces/zmk-modules/RolioFirmware/tools/build_rolio.sh -w /workspaces/zmk-modules/RolioFirmware/config/west.yml
```

| Option | Description |
| --- | --- |
| `-b`, `--build-script` | Path to the build wrapper script (required). |
| `-w`, `--west-yml` | Path to the `west.yml` file (required). |

Modules are found in the build script by looking for any `/workspaces/zmk-modules/<name>` path. Some differences are expected & are flagged as such rather than as errors:

- `RolioFirmware` (the repository itself) isn't listed in `west.yml`.
- `zmk` & `zephyr` are core repositories that don't belong in `ZMK_EXTRA_MODULES`.

<br/>

# Code Formatting

## `clean_code.sh`

The one to run before committing. It runs the three formatting tools below in order:

1. [`add_blank_lines.py`](#add_blank_linespy)
2. [`format.sh`](#formatsh)
3. [`format_keymap.py`](#format_keymappy) on `boards/shields/rolio/rolio.keymap`

```bash
/workspaces/zmk-modules/RolioFirmware/tools/clean_code.sh
```

All of these tools edit files in place, so it's a good idea to commit or stash your work first. Rebuild the firmware afterwards to make sure nothing was broken.

## `add_blank_lines.py`

Makes sure every top level function & struct in the C source under `boards/` is followed by two blank lines.

```bash
python3 /workspaces/zmk-modules/RolioFirmware/tools/add_blank_lines.py
```

Generated image, animation & font data (anything under an `assets` folder, plus `widgets/art.c`) is skipped, as it's overwritten whenever it is regenerated.

## `format.sh`

Runs `clang-format` over the `.c` & `.h` files under `boards/`, skipping the same generated files as above.

```bash
/workspaces/zmk-modules/RolioFirmware/tools/format.sh
```

The style is set inline (`IndentWidth: 4`, `ColumnLimit: 120`, `MaxEmptyLinesToKeep: 2`) rather than in a `.clang-format` file.

## `format_keymap.py`

Re-lays out the `bindings` of each layer in a keymap so they line up with the physical keys, drawn inside a box diagram of the Rolio46.

```bash
python3 /workspaces/zmk-modules/RolioFirmware/tools/format_keymap.py /workspaces/zmk-modules/RolioFirmware/boards/shields/rolio/rolio.keymap
```

- Only binding lists with 46 bindings (the standard layout) or 48 bindings (with the encoder push keys between the halves) are redrawn, anything else is left untouched.
- Column widths are shared across all layers & sized to fit the longest binding in each column, so every layer lines up.
- Runs of blank lines are collapsed & the layer nodes are re-indented.

<br/>

# Keymap Diagram

## `draw_keymap.sh` / `draw_keymap.py`

Draws `doc/images/keymap.svg`, the diagram shown in the [main README](../README.md#keymap-diagrams), from `rolio.keymap` & the physical layout in `rolio46-layout.dtsi` using [keymap-drawer](https://github.com/caksoylar/keymap-drawer). Run it whenever the keymap changes.

```bash
/workspaces/zmk-modules/RolioFirmware/tools/draw_keymap.sh
```

`draw_keymap.sh` is the entry point. The first time it runs it creates a Python virtual environment in `tools/venv` & installs the pinned keymap-drawer version into it, then it runs `draw_keymap.py`. Any arguments are passed through, for example to write the diagram somewhere else:

```bash
/workspaces/zmk-modules/RolioFirmware/tools/draw_keymap.sh --output /tmp/keymap.svg
```

On top of what keymap-drawer does by itself, `draw_keymap.py`:

- Gives custom behaviours & shortcuts friendly legends, with MacOS & Windows variants sharing the same legend.
- Uses Opt/Cmd on MacOS layers & Alt/Win on Windows layers.
- Adds the encoder rotations to the encoder keys.
- Highlights the key held to reach each layer, including layers reached through tap-dances.
- Merges `<name> (Mac)` & `<name> (Win)` layers into a single `<name>` diagram when they look identical.
- Styles the diagram to match the colour key in the main README.

If you add a new custom behaviour or encoder behaviour to the keymap, add a legend for it to `RAW_BINDINGS` or `SENSOR_BEHAVIOURS` in `draw_keymap.py`, otherwise the raw binding text will show up on the key. Likewise, if the encoder key positions move, update `ENCODER_POSITIONS`.

<br/>

# Display Assets

## `gif_to_lvgl.py`

Converts a GIF into a C file of 1-bit indexed (`LV_IMG_CF_INDEXED_1BIT`) LVGL images, the format used by the Vista508's icons & animations. You won't normally run it directly, it's called by the asset scripts described in the [Building Guide](../doc/building.md#development-tools):

- `boards/shields/vista508/src/assets/images/raw/update_c_files.sh` - The icons (converted with `--dual`).
- `boards/shields/vista508/src/assets/animations/generate_animations.sh` - The animations.

To run it by hand:

```bash
python3 /workspaces/zmk-modules/RolioFirmware/tools/gif_to_lvgl.py input.gif output.c --prefix my_image
```

| Option | Description |
| --- | --- |
| `gif_path` | The GIF to convert. |
| `c_path` | The C file to write. |
| `--prefix` | Prefix for the generated C variable names (defaults to `dance`). |
| `--threshold` | Greyscale threshold from 0 to 255 (defaults to `64`). Pixels at or below it are drawn black, brighter pixels are drawn white. |
| `--dual` | Also generate colour inverted copies of each frame (suffixed `_inv`). |

The generated file contains a `<prefix>_frame_<n>` image for each frame, a `<prefix>_frames[]` array of them & a `<prefix>_frame_count`. Only every 3rd frame is kept, turning a 30 FPS GIF into a 10 FPS animation.

The art shown on the right side's display is generated by a separate tool, see the [image converter](../image_converter/readme.md).

<br/>

# Generated Files

- `venv/` - The virtual environment created by `draw_keymap.sh`, safe to delete (it will be recreated on the next run).
- `__pycache__/` - Python's bytecode cache.

Both are ignored by git.
