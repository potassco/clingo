import os
import re
from pathlib import Path

from config import Config


def pkg_version() -> str | None:
    """
    Get the version from the PKG-INFO file if it exists, otherwise return None.
    """
    pkg_info = Path("PKG-INFO")
    if not pkg_info.is_file():
        return None
    m = re.search(
        r"^Version:\s*(\S+)\s*$", pkg_info.read_text(encoding="utf-8"), re.MULTILINE
    )
    return m.group(1) if m else None


def pypi_index() -> str:
    """
    Get the PyPI index to use for determining the build number.
    """
    extra_index_url = os.getenv("PIP_EXTRA_INDEX_URL", "").strip()
    if extra_index_url and "test.pypi.org" not in extra_index_url.lower():
        raise ValueError(
            "PIP_EXTRA_INDEX_URL is set but does not point to test.pypi.org"
        )
    return "test.pypi" if extra_index_url else "pypi"


def dynamic_metadata(_settings, _project):
    """
    Dynamically generate version number.

    Returns the version from the pkg_info file if it exists, otherwise
    generates a version number based on the base version and build number. The
    build number is determined by the number of builds on the PyPI index. If
    PYPI_EXTRA_INDEX_URL is set, it will use test.pypi.org as the index,
    otherwise it will use pypi.org.
    """
    version = pkg_version()
    if version is not None:
        return {"version": version}

    cfg = Config()
    base = cfg.get_version()

    env_num = os.getenv("BUILD_NUMBER")
    if env_num is None:
        num = 0
    elif env_num == "auto":
        index = pypi_index()
        num = cfg.pypi_build_number(index)
    else:
        num = int(env_num)

    build = f".post{num}" if num > 0 else ""

    return {"version": f"{base}{build}"}
