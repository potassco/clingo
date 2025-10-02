#!/usr/bin/env python

"""
Helper script for Profile Guided Optimization (PGO) with clang.
"""

import argparse
import os
import shlex
import subprocess
import sys
from pathlib import Path

CMAKE = "cmake"
CLANG = "clang-20"
CLANGXX = "clang++-20"
LLD = "lld-20"
LLVM_PROFDATA = "llvm-profdata-20"

BUILD_PATH_INSTRUMENT = "build/release_clang_instrument"
BUILD_PATH = "build/release_clang_pgo"
CMAKE_OPTIONS = [
    f"-DCMAKE_C_COMPILER={shlex.quote(CLANG)}",
    f"-DCMAKE_CXX_COMPILER={shlex.quote(CLANGXX)}",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCLINGO_BUILD_TESTS=Off",
    "-DCLINGO_BUILD_EXAMPLES=Off",
]
C_FLAGS = f"-flto -fuse-ld={shlex.quote(LLD)} -Wunused-command-line-argument"
CXX_FLAGS = f"-stdlib=libc++ {C_FLAGS}"


def collect_profraw_files(paths):
    """
    Collect all .profraw files from the given list of files and directories.

    Files must match the pattern profile-*.profraw. And directories are
    searched recursively for such files.
    """
    profraw_files = []
    for arg in paths:
        p = Path(arg)
        if p.is_dir():
            profraw_files.extend(p.rglob("profile-*.profraw"))
        elif (
            p.is_file()
            and p.name.startswith("profile-")
            and p.name.endswith(".profraw")
        ):
            profraw_files.append(p)
        else:
            print(f"Error: {arg} is not a valid file or directory.", file=sys.stderr)
            sys.exit(1)
    if not profraw_files:
        print("Error: No .profraw files found.", file=sys.stderr)
        sys.exit(1)
    return profraw_files


def run_command(cmd):
    """
    Run the given command and exit on failure.
    """
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {' '.join(cmd)}", file=sys.stderr)
        sys.exit(e.returncode)


def main():
    """
    Run the PGO helper script.
    """
    parser = argparse.ArgumentParser(
        description="PGO helper for clang: instrument, run, and build with profile data."
    )
    subparsers = parser.add_subparsers(dest="mode", required=True)

    # instrument
    subparsers.add_parser(
        "instrument", help="Configure and build with instrumentation."
    )

    # run
    run_parser = subparsers.add_parser(
        "run", help="Run instrumented binary with arguments."
    )
    run_parser.add_argument(
        "command", nargs=argparse.REMAINDER, help="Command and arguments to run."
    )

    # build
    build_parser = subparsers.add_parser(
        "build", help="Merge and use profile data for final build."
    )
    build_parser.add_argument(
        "inputs", nargs="+", help="profraw file(s) or directory(ies) to search"
    )

    args = parser.parse_args()

    if args.mode == "instrument":
        subprocess.run(["rm", "-rf", BUILD_PATH_INSTRUMENT], check=False)
        run_command(
            [
                CMAKE,
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                BUILD_PATH_INSTRUMENT,
                *CMAKE_OPTIONS,
                f"-DCMAKE_C_FLAGS=-fprofile-instr-generate {C_FLAGS}",
                f"-DCMAKE_CXX_FLAGS=-fprofile-instr-generate {CXX_FLAGS}",
            ]
        )
        run_command(["cmake", "--build", BUILD_PATH_INSTRUMENT])

    elif args.mode == "run":
        if not args.command:
            parser.error("the run command requires at least one argument")
        env = os.environ.copy()
        env["LLVM_PROFILE_FILE"] = "profile-%p.profraw"
        if os.name == "posix":
            os.execvpe(args.command[0], args.command, env)
        else:
            sys.exit(os.spawnvpe(os.P_WAIT, args.command[0], args.command, env))

    elif args.mode == "build":
        profraw_files = collect_profraw_files(args.inputs)
        merged = os.getcwd() + os.sep + "merged.profdata"
        subprocess.run(["rm", "-f", merged], check=False)
        run_command(
            [
                LLVM_PROFDATA,
                "merge",
                f"-output={merged}",
                *map(str, profraw_files),
            ]
        )
        subprocess.run(["rm", "-rf", BUILD_PATH], check=False)
        run_command(
            [
                CMAKE,
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                BUILD_PATH,
                *CMAKE_OPTIONS,
                f"-DCMAKE_C_FLAGS=-fprofile-instr-use={shlex.quote(merged)} {C_FLAGS}",
                f"-DCMAKE_CXX_FLAGS=-fprofile-instr-use={shlex.quote(merged)} {CXX_FLAGS}",
            ]
        )
        run_command([CMAKE, "--build", BUILD_PATH])
        subprocess.run(["rm", "-f", merged], check=False)


if __name__ == "__main__":
    main()
