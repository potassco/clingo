"""
Script to adjust version in pyproject.toml based on core.h version.
"""

import argparse
import re

import toml


def adjust_version(build_number, git_hash):
    """
    Adjust version in pyproject.toml.
    """

    version = None
    with open("lib/c-api/include/clingo/core.h", encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r'#define CLINGO_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)
    assert version is not None

    if build_number > 0:
        version = f"{version}.post{build_number}"

    with open("pyproject.toml", "r", encoding="utf-8") as hnd:
        pyproject = toml.load(hnd)

    pyproject["project"]["version"] = f"{version}+g{git_hash}"

    with open("pyproject.toml", "w", encoding="utf-8") as hnd:
        toml.dump(pyproject, hnd)

    print(f"Set version to {pyproject['project']['version']}")


def run():
    """
    Parse arguments and run version adjustment.
    """
    parser = argparse.ArgumentParser(description="Build source package.")
    parser.add_argument("--build-number", type=int, default=0, help="Build number.")
    parser.add_argument("--git-hash", type=str, required=True, help="Git hash.")
    args = parser.parse_args()

    adjust_version(args.build_number, args.git_hash)


if __name__ == "__main__":
    run()
