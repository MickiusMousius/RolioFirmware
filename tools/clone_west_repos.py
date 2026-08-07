#!/usr/bin/env python3

import os
import sys
import subprocess
import argparse
import yaml


def parse_args():
    parser = argparse.ArgumentParser(
        description="Clone or update repositories from a west.yml file to an explicit path."
    )
    parser.add_argument(
        "-m", "--manifest", required=True, help="Path to the west.yml file"
    )
    parser.add_argument(
        "-d", "--dest", required=True, help="Destination directory to clone into"
    )
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="Obliterate local changes and force updates",
    )
    return parser.parse_args()


def is_dirty(repo_path):
    """Check if the git repository has uncommitted changes."""
    result = subprocess.run(
        ["git", "status", "--porcelain"], cwd=repo_path, capture_output=True, text=True
    )
    return bool(result.stdout.strip())


def obliterate_changes(repo_path):
    """Forcefully reset and clean the repository."""
    print(f"   --> 💥 Obliterating local changes...")
    subprocess.run(
        ["git", "reset", "--hard"], cwd=repo_path, check=True, stdout=subprocess.DEVNULL
    )
    subprocess.run(
        ["git", "clean", "-fd"], cwd=repo_path, check=True, stdout=subprocess.DEVNULL
    )


def prompt_user(repo_path, message):
    """Pause and ask the user how to handle a conflict."""
    print(f"\n⚠️  WARNING: {message} in {repo_path}")
    while True:
        ans = (
            input(
                "   Type 's' to skip this repo, 'f' to force overwrite, or 'a' to abort entirely: "
            )
            .strip()
            .lower()
        )
        if ans == "s":
            return "skip"
        elif ans == "f":
            return "force"
        elif ans == "a":
            sys.exit("\n❌ Aborted by user.")
        else:
            print("   Invalid input. Please enter s, f, or a.")


def main():
    args = parse_args()
    manifest_path = os.path.abspath(args.manifest)
    dest_dir = os.path.abspath(args.dest)
    force_mode = args.force

    if not os.path.exists(manifest_path):
        sys.exit(f"Error: Manifest file not found at {manifest_path}")

    # Load the YAML file
    with open(manifest_path, "r") as f:
        try:
            manifest_data = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            sys.exit(f"Error parsing YAML: {exc}")

    manifest = manifest_data.get("manifest", {})

    # Map remotes
    remotes = {}
    for remote in manifest.get("remotes", []):
        name = remote.get("name")
        url_base = remote.get("url-base")
        if name and url_base:
            remotes[name] = url_base

    os.makedirs(dest_dir, exist_ok=True)

    # Repositories to ignore during the cloning/updating process
    EXCLUDED_REPOS = {"zmk"}

    projects = manifest.get("projects", [])
    for proj in projects:
        name = proj.get("name")

        # Skip explicitly excluded repositories
        if name in EXCLUDED_REPOS:
            print(f"\n⏭️  Skipping excluded repository: '{name}'")
            continue

        revision = proj.get("revision", "main")

        rel_path = proj.get("path", name)
        target_path = os.path.join(dest_dir, rel_path)

        # Build Git URL
        url = proj.get("url")
        if not url:
            remote_name = proj.get("remote")
            if remote_name and remote_name in remotes:
                repo_path = proj.get("repo-path", name)
                url = f"{remotes[remote_name]}/{repo_path}"
                if not url.endswith(".git"):
                    url += ".git"
            else:
                print(f"⚠️  Skipping '{name}': No explicit url or valid remote found.")
                continue

        print(f"\n📦 Processing: {name}")
        print(f"   Path:     {target_path}")
        print(f"   Revision: {revision}")

        # 1. NEW REPOSITORY
        if not os.path.exists(os.path.join(target_path, ".git")):
            print(f"   --> Not found locally. Cloning...")
            try:
                subprocess.run(["git", "clone", url, target_path], check=True)
                subprocess.run(
                    ["git", "checkout", revision],
                    cwd=target_path,
                    check=True,
                    stderr=subprocess.DEVNULL,
                )
                print(f"   ✅ Cloned and checked out {revision}")
            except subprocess.CalledProcessError:
                print(f"   ❌ Failed to clone or checkout {name}")
            continue

        # 2. EXISTING REPOSITORY UPDATE LOGIC
        print(f"   --> Directory exists. Checking state...")

        # Handle uncommitted changes
        if is_dirty(target_path):
            if force_mode:
                obliterate_changes(target_path)
            else:
                action = prompt_user(target_path, "Uncommitted changes found")
                if action == "skip":
                    print("   ⏭️  Skipping...")
                    continue
                elif action == "force":
                    obliterate_changes(target_path)

        # Fetch latest from remote
        print(f"   --> Fetching latest updates...")
        subprocess.run(
            ["git", "fetch", "origin", "--tags"],
            cwd=target_path,
            check=True,
            stdout=subprocess.DEVNULL,
        )
        subprocess.run(
            ["git", "fetch", "origin", revision],
            cwd=target_path,
            stderr=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
        )

        # Checkout the target revision
        try:
            subprocess.run(
                ["git", "checkout", revision],
                cwd=target_path,
                check=True,
                stderr=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
            )
        except subprocess.CalledProcessError:
            print(f"   ❌ Failed to checkout {revision}. Skipping.")
            continue

        # Check if the revision is a branch that needs to be fast-forwarded
        branch_check = subprocess.run(
            ["git", "rev-parse", "--verify", f"origin/{revision}"],
            cwd=target_path,
            capture_output=True,
            text=True,
        )

        if branch_check.returncode == 0:
            # It's a tracking branch, try to update it
            if force_mode:
                subprocess.run(
                    ["git", "reset", "--hard", f"origin/{revision}"],
                    cwd=target_path,
                    check=True,
                    stdout=subprocess.DEVNULL,
                )
                print(f"   ✅ Force reset to origin/{revision}")
            else:
                ff_check = subprocess.run(
                    ["git", "merge", "--ff-only", f"origin/{revision}"],
                    cwd=target_path,
                    capture_output=True,
                    text=True,
                )
                if ff_check.returncode == 0:
                    print(f"   ✅ Up to date at {revision}")
                else:
                    action = prompt_user(
                        target_path,
                        f"Branch '{revision}' has diverged from origin (needs rebase/merge)",
                    )
                    if action == "skip":
                        print("   ⏭️  Skipping...")
                        continue
                    elif action == "force":
                        subprocess.run(
                            ["git", "reset", "--hard", f"origin/{revision}"],
                            cwd=target_path,
                            check=True,
                            stdout=subprocess.DEVNULL,
                        )
                        print(f"   ✅ Force reset to origin/{revision}")
        else:
            # It's a specific commit SHA or tag, no merging required
            print(f"   ✅ Detached HEAD at {revision}")

    print(f"\n🎉 Finished processing all repositories into {dest_dir}")


if __name__ == "__main__":
    main()
