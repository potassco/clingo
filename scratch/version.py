import re
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print(
            f"usage: {Path(sys.argv[0]).name} <header-path> <macro-field>",
            file=sys.stderr,
        )
        return 2

    header_path = Path(sys.argv[1])
    field = sys.argv[2]

    pattern = re.compile(rf'#define\s+{re.escape(field)}\s+"([0-9]+\.[0-9]+\.[0-9]+)"')

    try:
        with header_path.open(encoding="utf-8") as fh:
            for line in fh:
                match = pattern.search(line)
                if match:
                    print(match.group(1))
                    return 0
    except OSError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"error: {field} not found in {header_path}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
