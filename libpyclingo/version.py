import json
import os
import re
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import urlopen


def pkg_version():
    pkg_info = Path("PKG-INFO")
    if not pkg_info.is_file():
        return None
    m = re.search(
        r"^Version:\s*(\S+)\s*$", pkg_info.read_text(encoding="utf-8"), re.MULTILINE
    )
    if not m:
        raise ValueError(f"Could not find version in {pkg_info}")
    return m.group(1)


def auto_build_number(package_name, base_version):
    extra_index_url = os.getenv("PIP_EXTRA_INDEX_URL", "").strip()
    if extra_index_url and "test.pypi.org" not in extra_index_url.lower():
        raise ValueError(
            "PIP_EXTRA_INDEX_URL is set but does not point to test.pypi.org"
        )
    use_test_pypi = bool(extra_index_url)
    index_base = (
        "https://test.pypi.org/pypi" if use_test_pypi else "https://pypi.org/pypi"
    )
    url = f"{index_base}/{quote(package_name)}/json"

    try:
        with urlopen(url, timeout=10) as response:
            data = json.loads(response.read().decode("utf-8"))
    except HTTPError as err:
        if err.code == 404:
            return 0
        raise
    except (URLError, TimeoutError, json.JSONDecodeError, OSError) as err:
        raise RuntimeError(f"Failed to fetch release metadata from {url}") from err

    releases = data.get("releases", {})
    if not isinstance(releases, dict):
        return 0

    pattern = re.compile(rf"^{re.escape(base_version)}(?:\.post([0-9]+))?$")
    found = []
    for version in releases:
        m = pattern.match(version)
        if m:
            found.append(int(m.group(1) or 0))

    return max(found) + 1 if found else 0


def dynamic_metadata(_settings, _project):
    version = pkg_version()
    if version is not None:
        return {"version": version}

    header = Path("libclingo/clingo.h")
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
    if build == "auto":
        package_name = _project.get("name", "clingo")
        build_number = auto_build_number(package_name, base)
    else:
        try:
            build_number = int(build) if build else 0
        except ValueError:
            raise ValueError(f'BUILD_NUMBER must be a number or "auto", got {build}')

    suffix = f".post{build_number}" if build_number > 0 else ""
    return {"version": f"{base}{suffix}"}
