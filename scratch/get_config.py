import sys
from configparser import ConfigParser
from pathlib import Path
from typing import Optional


def load_config(config_path: Path) -> ConfigParser:
    parser = ConfigParser()

    try:
        with config_path.open(encoding="utf-8") as fh:
            parser.read_file(fh)
    except OSError as exc:
        raise RuntimeError(f"cannot read config file {config_path}: {exc}") from exc

    return parser


def get_value(section: str, entry: str, default: Optional[str]) -> str:
    config_path = Path(__file__).resolve().with_name("config.ini")
    config = load_config(config_path)

    if section not in config:
        if default is not None:
            return default
        raise RuntimeError(f"missing section [{section}]")
    config_section = config[section]

    if entry not in config_section:
        if default is not None:
            return default
        raise RuntimeError(f"missing entry {entry}")
    return config_section[entry]


def main() -> int:
    try:
        if len(sys.argv) < 3 or len(sys.argv) > 4:
            raise RuntimeError("Usage: get_config.py <section> <entry> [default]")
        default = None
        if len(sys.argv) == 4:
            default = sys.argv[3]

        print(get_value(sys.argv[1], sys.argv[2], default))
        return 0
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
