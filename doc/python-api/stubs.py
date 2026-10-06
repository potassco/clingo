#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
from pathlib import Path

REFERENCE_RE = re.compile(r"`(?P<name>clingo(?:\.[A-Za-z_][A-Za-z0-9_]*)*)`")


def transform(text: str) -> str:
    def replace(match: re.Match[str]) -> str:
        name = match.group("name")
        display_name = name.split(".")[-1]

        return f"[`{display_name}`][{name}]"

    return REFERENCE_RE.sub(replace, text)


def process_tree(source: Path, destination: Path) -> int:
    count = 0

    for source_file in source.rglob("*.pyi"):
        relative = source_file.relative_to(source)
        destination_file = destination / relative

        destination_file.parent.mkdir(parents=True, exist_ok=True)

        original = source_file.read_text(encoding="utf-8")
        transformed = transform(original)

        destination_file.write_text(transformed, encoding="utf-8")

        if original != transformed:
            count += 1
            print(f"transformed: {relative}")
        else:
            print(f"copied:      {relative}")

    return count


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Copy .pyi stubs while converting clingo references to links."
    )
    parser.add_argument("source", type=Path, help="Source stub directory")
    parser.add_argument("destination", type=Path, help="Destination stub directory")
    args = parser.parse_args()

    if not args.source.is_dir():
        parser.error(f"Source directory does not exist: {args.source}")

    changed = process_tree(args.source, args.destination)
    print(f"\nChanged {changed} file(s).")


if __name__ == "__main__":
    main()
