#!/usr/bin/env python3

import json
import sys
import urllib.error
import urllib.request

OWNER = "potassco"


def next_build_number(package: str, version: str, label: str) -> int:
    url = f"https://api.anaconda.org/package/{OWNER}/{package}/files"

    try:
        with urllib.request.urlopen(url) as response:
            files = json.load(response)
    except (urllib.error.URLError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"Could not retrieve package files: {exc}") from exc

    build_numbers = [
        entry["attrs"]["build_number"]
        for entry in files
        if entry.get("version") == version
        and label in entry.get("labels", [])
        and isinstance(entry.get("attrs", {}).get("build_number"), int)
    ]

    return max(build_numbers) + 1 if build_numbers else 0


def main() -> int:
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} PACKAGE VERSION LABEL", file=sys.stderr)
        return 2

    package, version, label = sys.argv[1:]
    try:
        print(next_build_number(package, version, label))
    except RuntimeError as exc:
        print(exc, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
