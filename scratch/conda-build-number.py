#!/usr/bin/env python3

import configparser
import json
import sys
import urllib.error
import urllib.request
from pathlib import Path

from version import get_version


def load_conda_config(config_path: Path) -> tuple[str, str]:
    parser = configparser.ConfigParser()

    try:
        with config_path.open(encoding="utf-8") as fh:
            parser.read_file(fh)
    except OSError as exc:
        raise RuntimeError(f"cannot read config file {config_path}: {exc}") from exc

    if "conda" not in parser:
        raise RuntimeError(f"missing [conda] section in {config_path}")

    section = parser["conda"]
    name = section.get("name")
    owner = section.get("owner")

    if not name or not owner:
        raise RuntimeError(f"{config_path}: [conda] must define name and owner")

    return owner, name


def next_build_number(owner: str, package: str, version: str, label: str) -> int:
    url = f"https://api.anaconda.org/package/{owner}/{package}/files"

    try:
        with urllib.request.urlopen(url) as response:
            files = json.load(response)
    except (urllib.error.URLError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"could not retrieve package files: {exc}") from exc

    # non-main packages always get a fresh build number
    # main packages can override dev packages
    build_numbers = [
        entry["attrs"]["build_number"]
        for entry in files
        if entry.get("version") == version
        and (label != "main" or "main" in entry.get("labels", []))
        and isinstance(entry.get("attrs", {}).get("build_number"), int)
    ]

    return max(build_numbers) + 1 if build_numbers else 0


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} LABEL", file=sys.stderr)
        return 2

    label = sys.argv[1]
    config_path = Path(__file__).resolve().with_name("config.ini")

    try:
        owner, package = load_conda_config(config_path)
        version = get_version()
        print(next_build_number(owner, package, version, label))
    except RuntimeError as exc:
        print(exc, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
