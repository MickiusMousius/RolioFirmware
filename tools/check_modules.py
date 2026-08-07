#!/usr/bin/env python3

import os
import re
import argparse
import yaml
import sys


def parse_args():
    parser = argparse.ArgumentParser(
        description="Cross-reference a bash build script and a west.yml file for module consistency."
    )
    parser.add_argument(
        "-b", "--build-script", required=True, help="Path to your build wrapper script"
    )
    parser.add_argument(
        "-w", "--west-yml", required=True, help="Path to your west.yml file"
    )
    return parser.parse_args()


def get_bash_modules(script_path):
    """Extracts module directory names from the bash script."""
    modules = set()
    with open(script_path, "r") as f:
        content = f.read()

    # Looks for /workspaces/zmk-modules/ followed by standard folder characters
    matches = re.findall(r"/workspaces/zmk-modules/([a-zA-Z0-9_-]+)", content)
    for match in matches:
        modules.add(match)

    return modules


def get_yaml_modules(yaml_path):
    """Extracts expected directory paths from west.yml."""
    modules = set()
    with open(yaml_path, "r") as f:
        try:
            manifest_data = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            print(f"❌ Error parsing YAML: {exc}")
            sys.exit(1)

    if not manifest_data or "manifest" not in manifest_data:
        print(
            f"❌ Error: Could not find 'manifest' key in {yaml_path}. Is it a valid west.yml?"
        )
        sys.exit(1)

    projects = manifest_data.get("manifest", {}).get("projects", [])
    for proj in projects:
        name = proj.get("name")
        folder_name = proj.get("path", name)
        if folder_name:
            modules.add(folder_name)

    return modules


def main():
    args = parse_args()

    if not os.path.exists(args.build_script):
        print(f"❌ Could not find build script at {args.build_script}")
        sys.exit(1)

    if not os.path.exists(args.west_yml):
        print(f"❌ Could not find west.yml at {args.west_yml}")
        sys.exit(1)

    bash_modules = get_bash_modules(args.build_script)
    yaml_modules = get_yaml_modules(args.west_yml)

    print(
        f"🔍 Analyzing '{os.path.basename(args.build_script)}' against '{os.path.basename(args.west_yml)}'...\n"
    )

    print(f"👀 Found {len(bash_modules)} modules in Bash script.")
    print(f"👀 Found {len(yaml_modules)} projects in west.yml.\n")

    missing_in_yaml = bash_modules - yaml_modules
    missing_in_bash = yaml_modules - bash_modules

    # Repos in Bash script, but normal to be missing from YAML
    EXPECTED_MISSING_FROM_YAML = {"RolioFirmware"}

    # Repos in YAML, but normal to be missing from Bash's EXTRA_MODULES
    EXPECTED_MISSING_FROM_BASH = {"zmk", "zephyr"}

    if missing_in_yaml:
        print("⚠️  Modules in build script, but MISSING from west.yml:")
        for mod in sorted(missing_in_yaml):
            if mod in EXPECTED_MISSING_FROM_YAML:
                print(
                    f"   - {mod} (ℹ️  Note: Normal for this base/manifest repo to be missing)"
                )
            else:
                print(f"   - {mod} ❌ (Will fail if not manually cloned)")
    else:
        print("✅ No orphaned modules in the build script.")

    print("")

    if missing_in_bash:
        print("⚠️  Modules in west.yml, but MISSING from build script:")
        for mod in sorted(missing_in_bash):
            if mod in EXPECTED_MISSING_FROM_BASH:
                print(
                    f"   - {mod} (ℹ️  Note: Normal. Core repositories don't go in ZMK_EXTRA_MODULES)"
                )
            else:
                print(f"   - {mod} ❌ (West downloads this, but CMake won't use it!)")
    else:
        print("✅ Build script includes all extra modules downloaded by West.")

    print("\n🏁 Done.")


if __name__ == "__main__":
    main()
