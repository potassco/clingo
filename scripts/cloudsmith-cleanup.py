#!/usr/bin/env python3

import subprocess as sp
import sys
import json
import re
import argparse
from packaging import version
from collections import defaultdict

ACCOUNT = "potassco"
REPO = "wip-20"
DRY_RUN = True


def get_packages():
    """Fetch all packages from Cloudsmith as JSON"""
    cmd = [
        "cloudsmith",
        "list",
        "packages",
        f"{ACCOUNT}/{REPO}",
        "--show-all",
        "-F",
        "json",
    ]
    result = sp.run(cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        print(f"Error fetching packages: {result.stderr}")
        sys.exit(1)
    return json.loads(result.stdout)


def group_by_name(packages_data):
    """Group packages by name, mapping to list of (version, slug_perm)"""
    grouped = defaultdict(list)
    for pkg in packages_data.get("data", []):
        name = pkg["name"]
        ver = pkg["version"]
        slug = pkg["slug_perm"]
        tp = pkg["type_display"]
        grouped[(name, tp)].append({"version": ver, "slug_perm": slug})
    return grouped


def convert_version(version_str: str):
    return re.sub(r"-([a-z]+)(\d+)", r".\2", version_str)


def sort_versions(version_list):
    """Sort a list of version dicts by version number"""
    try:
        return sorted(
            version_list, key=lambda x: version.parse(convert_version(x["version"]))
        )
    except Exception as e:
        print(f"Warning: Could not parse versions with packaging library: {e}")
        # Fallback to string sort
        return sorted(version_list, key=lambda x: x["version"])


def delete_package(pkg):
    """Delete a package by slug"""
    slug_perm = pkg["slug_perm"]
    version = pkg["version"]
    cmd = ["cloudsmith", "delete", f"{ACCOUNT}/{REPO}/{slug_perm}", "--yes"]
    result = sp.run(cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        print(f"  ❌ Error deleting {version} {slug_perm}: {result.stderr}")
        return False
    else:
        print(f"  ✓ Deleted {version} ({slug_perm})")
        return True


def run(exec: bool):
    print(f"Fetching packages from {ACCOUNT}/{REPO}...")
    packages_data = get_packages()

    grouped = group_by_name(packages_data)

    total_to_delete = 0
    total_deleted = 0

    for key in sorted(grouped.keys()):
        pkg_name = key[0]
        type_display = key[1]
        versions = grouped[key]
        sorted_versions = sort_versions(versions)

        if sorted_versions:
            latest = sorted_versions[-1]
            to_delete = sorted_versions[:-1]

            print(f"\n{pkg_name} ({type_display}):")
            print(f"  Keeping: {latest['version']} ({latest['slug_perm']})")

            for old_pkg in to_delete:
                total_to_delete += 1
                if exec:
                    if delete_package(old_pkg["slug_perm"]):
                        total_deleted += 1
                else:
                    print(
                        f"  ℹ️ Would delete {old_pkg['version']} ({old_pkg['slug_perm']})"
                    )
                    total_deleted += 1
    pre = "Delete" if exec else "Would delete"
    print(f"\n\nSummary: {pre} {total_deleted}/{total_to_delete} old package versions")


def main():
    parser = argparse.ArgumentParser(
        description="Delete old package versions from Cloudsmith, keeping only the latest of each variant."
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Execute deletions for real. Default is dry-run mode.",
    )

    args = parser.parse_args()
    run(args.execute)


if __name__ == "__main__":
    main()
