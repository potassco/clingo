import argparse
import configparser
import os
import shlex
import subprocess
import sys
import tempfile

EXAMPLE_CONFIG = """
[general]
file_path = bug.lp

[patterns]
pat1 = UNSATISFIABLE
pat2 = 1+

[commands]
cmd1 = clingo-5.8 --quiet
cmd2 = clingo-6.0 --quiet
"""


def generate_example_config(config_path):
    with open(config_path, "w", encoding="utf-8") as f:
        f.write(EXAMPLE_CONFIG.strip())
    print(f"Example config file generated at {config_path}")
    sys.exit(0)


def parse_command(cmd_str):
    return shlex.split(cmd_str)


def run_command(cmd, filename):
    process = subprocess.run(
        cmd + [filename], capture_output=True, text=True, check=False
    )
    return process.stdout


def process_file(filename, cmd1, cmd2, pat1, pat2, block_size=1):
    with open(filename, "r", encoding="utf-8") as f:
        original_lines = f.readlines()

    current_lines = original_lines.copy()
    total_lines = len(current_lines)
    i = 0

    with tempfile.NamedTemporaryFile("w+") as tmp:
        tmp_name = tmp.name
        print(f"Using temporary file: {tmp_name}")

        while i < total_lines:
            test_lines = current_lines[:i] + current_lines[i + block_size :]
            tmp.seek(0)
            tmp.truncate()
            tmp.writelines(test_lines)
            tmp.flush()

            out1 = run_command(cmd1, tmp_name)
            out2 = run_command(cmd2, tmp_name)

            if pat1 in out1 and pat2 in out2:
                print(f"Removing block starting at line {i + 1}")
                current_lines = test_lines
                total_lines -= block_size
            else:
                print(f"Keeping block starting at line {i + 1}")
                i += block_size

    with open(filename, "w", encoding="utf-8") as f:
        f.writelines(current_lines)


def main():
    parser = argparse.ArgumentParser(
        description="Reduce a file using two commands and patterns from config."
    )
    parser.add_argument("config", help="Path to config file")
    parser.add_argument(
        "--block-size",
        type=int,
        default=1,
        help="Block size for reduction (default: 1)",
    )
    args = parser.parse_args()

    if not os.path.exists(args.config):
        generate_example_config(args.config)

    config = configparser.ConfigParser()
    config.read(args.config)

    file_path = config.get("general", "file_path")
    pat1 = config.get("patterns", "pat1")
    pat2 = config.get("patterns", "pat2")
    cmd1 = parse_command(config.get("commands", "cmd1"))
    cmd2 = parse_command(config.get("commands", "cmd2"))

    process_file(file_path, cmd1, cmd2, pat1, pat2, block_size=args.block_size)


if __name__ == "__main__":
    main()
