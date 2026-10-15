import configparser
import re
import sys
from pathlib import Path


def load_config(config_path: Path) -> tuple[Path, str]:
    parser = configparser.ConfigParser()

    try:
        with config_path.open(encoding="utf-8") as fh:
            parser.read_file(fh)
    except OSError as exc:
        raise RuntimeError(f"cannot read config file {config_path}: {exc}") from exc

    if "version" not in parser:
        raise RuntimeError(f"missing [version] section in {config_path}")

    section = parser["version"]
    header_raw = section.get("header_path")
    field = section.get("macro_field")

    if not header_raw or not field:
        raise RuntimeError(
            f"{config_path}: [version] must define header_path and macro_field"
        )

    header_path = Path(header_raw)
    if not header_path.is_absolute():
        header_path = (config_path.parent / header_path).resolve()

    return header_path, field


def get_version() -> str:
    config_path = Path(__file__).resolve().with_name("config.ini")

    header_path, field = load_config(config_path)

    pattern = re.compile(rf'#define\s+{re.escape(field)}\s+"([0-9]+\.[0-9]+\.[0-9]+)"')

    try:
        with header_path.open(encoding="utf-8") as fh:
            for line in fh:
                match = pattern.search(line)
                if match:
                    return match.group(1)
    except OSError as exc:
        raise RuntimeError(f"cannot read header file {header_path}: {exc}") from exc

    raise RuntimeError(f"{field} not found in {header_path}")


def main() -> int:
    try:
        print(get_version())
        return 0
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
