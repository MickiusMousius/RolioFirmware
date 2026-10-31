#!/usr/bin/env python3
"""
Draws the keymap diagram (doc/images/keymap.svg) from the Rolio46 keymap using keymap-drawer.

keymap-drawer parses the keymap, then the legends are tidied up so they read nicely: custom behaviours
get friendly names, encoder rotations are added to the encoder keys & MacOS/Windows variants of a layer
that do the same thing are merged into a single diagram.

Run it via draw_keymap.sh so that keymap-drawer is installed.
"""

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml

FIRMWARE_DIR = Path(__file__).resolve().parent.parent
SHIELD_DIR = FIRMWARE_DIR / "boards" / "shields" / "rolio"
KEYMAP = SHIELD_DIR / "rolio.keymap"
PHYSICAL_LAYOUT = SHIELD_DIR / "rolio46-layout.dtsi"
OUTPUT = FIRMWARE_DIR / "doc" / "images" / "keymap.svg"

# Key positions of the encoder push buttons, in the same order as the sensors in rolio.dtsi
ENCODER_POSITIONS = (30, 31)

# Layers named "<name> (Mac)" & "<name> (Win)" are drawn once as "<name>" when their legends match
OS_SUFFIX = re.compile(r"\s*\((Mac|Win)\)$")

TRANS = {"t": "▽", "type": "trans"}

# Legends for bindings keymap-drawer can't name nicely, MacOS & Windows variants share a legend
RAW_BINDINGS = {
    "&mt LA(BSPC) BSPC": {"t": "BSPC", "h": "Del Word"},
    "&mt LC(BSPC) BSPC": {"t": "BSPC", "h": "Del Word"},
    "&scw RSHIFT 0": {"t": "Caps Word", "h": "Shift"},
    "&td1": {"t": "Symbols", "s": "2× NumPad", "type": "layer"},
    "&td2": {"t": "Symbols", "s": "2× NumPad", "type": "layer"},
    "&td2_config": {"t": "Settings", "s": "2× Lock", "type": "layer"},
    "&tog NUMPAD": "Num Pad Off",
    "&kp LC(LG(SPACE))": "Emoji",
    "&kp LG(DOT)": "Emoji",
    "&usb_tog OUT_TOG ESCAPE": {"t": "Esc", "h": "USB/BT"},
    **{f"&btkp {i} F{i + 1}": {"t": f"F{i + 1}", "h": f"BT {i + 1}"} for i in range(5)},
    "&mac_talk": "Dictate",
    "&kp LG(LEFT)": "Line Start",
    "&kp HOME": "Line Start",
    "&kp LG(RIGHT)": "Line End",
    "&kp END": "Line End",
    "&mac_cap_app": "Capture App",
    "&kp LA(PSCRN)": "Capture App",
    "&kp LG(LS(N4))": "Capture Area",
    "&kp LG(LS(S))": "Capture Area",
    "&bl BL_TOG": "Light Toggle",
    "&bl BL_SET 4": "Light Min",
    "&bl BL_INC": "Light Up",
    "&bl BL_DEC": "Light Down",
    "&mkp LCLK": "Left Click",
    "&output_usb": "USB",
    **{f"&output_bt {i}": f"BT {i + 1}" for i in range(5)},
    "&btclr_kp X X": {"t": "X", "h": "Clear BT"},
    "&soft_off": "Soft Off",
    "&wpm_tog": "WPM Toggle",
    "&mod_tog": "Mod Toggle",
    "&df_win": "Win Layout",
    "&df_mac": "Mac Layout",
}

# Friendlier names for the legends keymap-drawer generates
KEY_NAMES = {
    "ESCAPE": "Esc",
    "TAB": "Tab",
    "Sft+TAB": "Shift Tab",
    "ENTER": "Enter",
    "SPACE": "Space",
    "DEL": "Del",
    "CAPS": "Caps Lock",
    "APP": "Menu",
    "UP": "Up",
    "DOWN": "Down",
    "LEFT": "Left",
    "RIGHT": "Right",
    "PG UP": "Page Up",
    "PG DN": "Page Down",
    "PREV": "Prev Track",
    "PP": "Play Pause",
    "NEXT": "Next Track",
    "VOL UP": "Vol Up",
    "VOL DN": "Vol Down",
    "MUTE": "Mute",
    "KP DIVIDE": "/",
    "KP MULTIPLY": "*",
    "KP DOT": ".",
    "LSHIFT": "Shift",
    "RSHIFT": "Shift",
    "LCTRL": "Ctrl",
    "RCTRL": "Ctrl",
    "Number Pad": "Num Pad",
}
MAC_MODS = {"LALT": "Opt", "RALT": "Opt", "LGUI": "Cmd", "RGUI": "Cmd"}
WIN_MODS = {"LALT": "Alt", "RALT": "Alt", "LGUI": "Win", "RGUI": "Win"}

# Encoder rotation legends (clockwise, counter clockwise)
SENSOR_BEHAVIOURS = {
    "&mac_vol": ("Vol", "Vol"),
    "&scroll_up_down": ("Scroll", "Scroll"),
    "&bri_adjust": ("Bright", "Dim"),
}
SENSOR_KEYS = {"C_VOL_UP": "Vol", "C_VOL_DN": "Vol"}

# Matches the look of the original Inkscape diagram & the colour key in the README
DRAW_CONFIG = {
    "key_w": 56,
    "key_h": 56,
    "outer_pad_w": 30,
    "outer_pad_h": 50,
    "shrink_wide_legends": 6,
    "style_layer_activators": False,
    "svg_extra_style": """
svg.keymap { fill: #d1d6db; }
rect.key { fill: #3f4750; }
rect.key, rect.combo { stroke: #60666c; }
rect.held { fill: #ff00ff; }
text.held { fill: #24292e; font-weight: bold; }
text.hold { fill: #00ffff; }
text.layer { fill: #00ffff; }
text.layer.shifted { fill: #d1d6db; }
rect.encoder { fill: #2b3138; stroke: #9aa0a6; stroke-dasharray: 3, 2; }
text.encoder.shifted, text.encoder.hold { fill: #ff8cff; }
text.label, text.footer { stroke: black; }
text.trans { fill: #7e8184; }
""",
}


def parse_args():
    parser = argparse.ArgumentParser(description="Draw the Rolio46 keymap diagram using keymap-drawer.")
    parser.add_argument("-o", "--output", type=Path, default=OUTPUT, help=f"SVG to write (default: {OUTPUT})")
    return parser.parse_args()


def strip_comments(text):
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)


def split_bindings(text):
    """Splits a devicetree binding list into one normalised string per binding, e.g. '&kp A'."""
    return [" ".join(b.split()) for b in re.findall(r"&[^&]+", text.replace("<", " ").replace(">", " "))]


def read_keymap():
    """Returns the raw bindings & sensor bindings of each layer, along with the layer number defines."""
    text = strip_comments(KEYMAP.read_text())
    defines = {name: int(num) for name, num in re.findall(r"#define\s+(\w+)\s+(\d+)\b", text)}
    keymap = text[text.index('"zmk,keymap"') :]

    # Expand binding shorthands such as "#define ___ &trans"
    for name, value in re.findall(r"#define\s+(\S+)\s+(&[^\n]*)", text):
        keymap = re.sub(rf"(?<![\w&]){re.escape(name)}(?!\w)", value.strip(), keymap)

    layers = []
    for body in re.findall(r"\w+\s*\{([^{}]*)\}", keymap):
        bindings = re.search(r"(?<![\w-])bindings\s*=\s*<(.*?)>;", body, re.S)
        if not bindings:
            continue
        sensors = re.search(r"sensor-bindings\s*=\s*<(.*?)>;", body, re.S)
        layers.append(
            {
                "bindings": split_bindings(bindings.group(1)),
                "sensors": split_bindings(sensors.group(1)) if sensors else [],
            }
        )
    return layers, defines


def read_tap_dance_layers(defines):
    """Maps each tap-dance whose first binding is '&mo LAYER' to that layer's number."""
    keymap = KEYMAP.read_text()
    text = "".join(strip_comments((SHIELD_DIR / inc).read_text()) for inc in re.findall(r'#include\s+"([^"]+)"', keymap)
                   if (SHIELD_DIR / inc).exists())

    tap_dances = {}
    for label, body in re.findall(r"(\w+):\s*[\w-]+\s*\{([^{}]*)\}", text):
        momentary = re.search(r"(?<![\w-])bindings\s*=\s*<&mo\s+(\w+)>", body)
        if "zmk,behavior-tap-dance" in body and momentary:
            layer = momentary.group(1)
            tap_dances[f"&{label}"] = activated_layer(f"&mo {layer}", defines, {})
    return tap_dances


def run_keymap_drawer(*args):
    return subprocess.run([sys.executable, "-m", "keymap_drawer", *args], check=True, capture_output=True,
                          text=True).stdout


def parse_with_keymap_drawer(tmp_dir):
    config = tmp_dir / "parse_config.yaml"
    config.write_text(yaml.safe_dump({"parse_config": {"raw_binding_map": RAW_BINDINGS}}, allow_unicode=True))
    parsed = yaml.safe_load(run_keymap_drawer("-c", str(config), "parse", "-z", str(KEYMAP), "-c", "0"))

    flatten = lambda rows: [k for row in rows for k in (row if isinstance(row, list) else [row])]
    return [(name, flatten(rows)) for name, rows in parsed["layers"].items()]


def rename(legend, names):
    if isinstance(legend, str):
        return OS_SUFFIX.sub("", names.get(legend, legend))
    return {field: rename(value, names) if field in "tsh" else value for field, value in legend.items()}


def tap(legend):
    return legend if isinstance(legend, str) else legend.get("t", "")


def sensor_legends(sensor):
    behaviour, *params = sensor.split()
    if behaviour in SENSOR_BEHAVIOURS:
        return SENSOR_BEHAVIOURS[behaviour]
    if behaviour == "&inc_dec_kp" and len(params) == 2:
        return tuple(SENSOR_KEYS.get(p, KEY_NAMES.get(p, p.title())) for p in params)
    return (sensor, sensor)


def activated_layer(binding, defines, tap_dances):
    """Returns the layer number a binding momentarily activates, if any."""
    if binding in tap_dances:
        return tap_dances[binding]
    behaviour, *params = binding.split()
    if behaviour in ("&mo", "&lt") and params:
        return defines.get(params[0], int(params[0]) if params[0].isdigit() else None)
    return None


def build_layers(drawer_layers, keymap_layers, defines, tap_dances):
    layers = []
    for name, keys in drawer_layers:
        names = {**KEY_NAMES, **(MAC_MODS if "mac" in name.lower() else WIN_MODS)}
        layers.append((name, [rename(k, names) for k in keys]))

    # Encoder rotations are shown on base layers (no transparent keys) & wherever they differ from the first layer
    base_sensors = keymap_layers[0]["sensors"]
    for (name, keys), raw in zip(layers, keymap_layers):
        is_base = "&trans" not in raw["bindings"]
        for sensor, base_sensor, pos in zip(raw["sensors"], base_sensors, ENCODER_POSITIONS):
            if is_base or sensor != base_sensor:
                cw, ccw = sensor_legends(sensor)
                key = {"t": tap(keys[pos])} if keys[pos] != TRANS else {"t": "▽"}
                keys[pos] = {**key, "s": f"▲ {cw}", "h": f"▼ {ccw}", "type": "encoder"}

    # keymap-drawer marks the keys holding a layer for &mo & &lt, add the tap-dances & label the held keys
    for i, (name, keys) in enumerate(layers):
        for (_, other_keys), raw in zip(layers, keymap_layers):
            for pos, binding in enumerate(raw["bindings"]):
                if activated_layer(binding, defines, tap_dances) != i:
                    continue
                if keys[pos] == TRANS or keys[pos] == {"type": "held"}:
                    keys[pos] = {"t": tap(other_keys[pos]), "type": "held"}

    # Merge MacOS & Windows variants of a layer that look identical
    merged = {}
    for name, keys in layers:
        merged.setdefault(OS_SUFFIX.sub("", name), []).append((name, keys))
    result = {}
    for base_name, variants in merged.items():
        if all(keys == variants[0][1] for _, keys in variants):
            result[base_name] = variants[0][1]
        else:
            result.update(variants)

    # Stack multi-word tap legends so they fit on a key
    for keys in result.values():
        for pos, key in enumerate(keys):
            if isinstance(key, str):
                keys[pos] = key.replace(" ", "\n")
            elif "t" in key:
                key["t"] = key["t"].replace(" ", "\n")
    return result


def main():
    args = parse_args()

    keymap_layers, defines = read_keymap()
    tap_dances = read_tap_dance_layers(defines)

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        drawer_layers = parse_with_keymap_drawer(tmp_dir)
        if len(drawer_layers) != len(keymap_layers):
            sys.exit(f"❌ keymap-drawer found {len(drawer_layers)} layers but {len(keymap_layers)} were expected")
        for (name, keys), raw in zip(drawer_layers, keymap_layers):
            if len(keys) != len(raw["bindings"]):
                sys.exit(f"❌ Layer '{name}' has {len(keys)} keys but {len(raw['bindings'])} bindings were read")

        layers = build_layers(drawer_layers, keymap_layers, defines, tap_dances)

        keymap_yaml = tmp_dir / "keymap.yaml"
        keymap_yaml.write_text(
            yaml.safe_dump({"layout": {"dts_layout": str(PHYSICAL_LAYOUT)}, "layers": layers}, allow_unicode=True,
                           sort_keys=False)
        )
        draw_config = tmp_dir / "draw_config.yaml"
        draw_config.write_text(yaml.safe_dump({"draw_config": DRAW_CONFIG}, allow_unicode=True))
        svg = run_keymap_drawer("-c", str(draw_config), "draw", str(keymap_yaml))

    args.output.write_text(svg)
    print(f"✅ Drew {len(layers)} layers ({', '.join(layers)}) to {args.output}")


if __name__ == "__main__":
    main()
