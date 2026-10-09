import os
import re
from pathlib import Path


def pkg_version() -> str | None:
    pkg_info = Path("PKG-INFO")
    if not pkg_info.is_file():
        return None
    m = re.search(
        r"^Version:\s*(\S+)\s*$", pkg_info.read_text(encoding="utf-8"), re.MULTILINE
    )
    return m.group(1) if m else None


def dynamic_metadata(_settings, _project):
    version = pkg_version()
    if version is not None:
        return {"version": version}

    header = Path("lib/c-api/include/clingo/core.h")
    if not header.is_file():
        raise ValueError(f"Version header not found: {header}")
    text = header.read_text(encoding="utf-8")

    match = re.search(
        r'^\s*#define\s+CLINGO_VERSION\s+"([0-9]+(?:\.[0-9]+)*)"', text, re.MULTILINE
    )
    if match is None:
        raise ValueError(f"Could not find CLINGO_VERSION in {header}")
    base = match.group(1)

    build = os.getenv("BUILD_NUMBER", "").strip()
    try:
        build_number = int(build) if build else 0
    except ValueError:
        raise ValueError(f"BUILD_NUMBER must be a number, got {build}")
    build = f".post{build_number}" if build_number > 0 else ""

    return {"version": f"{base}{build}"}
