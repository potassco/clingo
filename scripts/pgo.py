#!/usr/bin/env python

"""
Helper script for Profile Guided Optimization (PGO) with clang.
"""

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

def command(cmd, alt):
    return cmd if shutil.which(cmd) is not None else alt

CMAKE = "cmake"
CLANG = command("clang-20", "clang")
CLANGXX = command("clang++-20", "clang++")
LLD = command("lld-20", "lld")
LLVM_PROFDATA = command("llvm-profdata-20", "llvm-profdata")
GCC = "gcc"
GXX = "g++"
GCOV_TOOL = "gcov-tool"

if CONDA_PREFIX := os.environ.get("CONDA_PREFIX", ""):
    CONDA_LIB_PATH = f"{shlex.quote(CONDA_PREFIX)}/lib"
    EXTRA_FLAGS = f" -L{CONDA_LIB_PATH} -Wl,-rpath={CONDA_LIB_PATH}"
else:
    EXTRA_FLAGS = ""

CMAKE_OPTIONS = [
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCLINGO_BUILD_TESTS=Off",
    "-DCLINGO_BUILD_EXAMPLES=Off",
]

CLANG_INSTR_PATH = "build/release_clang_instrument"
CLANG_BUILD_PATH = "build/release_clang_pgo"
CLANG_CMAKE_OPTIONS = [
    f"-DCMAKE_C_COMPILER={shlex.quote(CLANG)}",
    f"-DCMAKE_CXX_COMPILER={shlex.quote(CLANGXX)}",
    *CMAKE_OPTIONS,
]
CLANG_C_FLAGS = f"-flto -fuse-ld={shlex.quote(LLD)} -Wno-unused-command-line-argument{EXTRA_FLAGS}"
CLANG_CXX_FLAGS = f"-stdlib=libc++ {CLANG_C_FLAGS}"

GCC_INSTR_PATH = "build/release_instrument"
GCC_BUILD_PATH = "build/release_pgo"
GCC_CMAKE_OPTIONS = [
    f"-DCMAKE_C_COMPILER={shlex.quote(GCC)}",
    f"-DCMAKE_CXX_COMPILER={shlex.quote(GXX)}",
    *CMAKE_OPTIONS,
]
GCC_C_FLAGS = "-flto=2 -fuse-linker-plugin"
GCC_CXX_FLAGS = GCC_C_FLAGS


def collect_profraw_files(paths):
    """
    Collect all .profraw files from the given list of files and directories.

    Files must match the pattern default.profraw. And directories are
    searched recursively for such files.
    """
    profraw_files = []
    for arg in paths:
        p = Path(arg)
        if p.is_dir():
            profraw_files.extend(p.rglob("default.profraw"))
        elif p.is_file() and p.name == "default.profraw":
            profraw_files.append(p)
        else:
            print(f"Error: {arg} is not a valid file or directory.", file=sys.stderr)
            sys.exit(1)
    if not profraw_files:
        print("Error: No default.profraw files found.", file=sys.stderr)
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


def clang_instrument():
    """
    Instrument the build with clang.
    """
    shutil.rmtree(CLANG_INSTR_PATH, ignore_errors=True)
    run_command(
        [
            CMAKE,
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            CLANG_INSTR_PATH,
            *CLANG_CMAKE_OPTIONS,
            f"-DCMAKE_C_FLAGS=-fprofile-instr-generate {CLANG_C_FLAGS}",
            f"-DCMAKE_CXX_FLAGS=-fprofile-instr-generate {CLANG_CXX_FLAGS}",
        ]
    )
    run_command(["cmake", "--build", CLANG_INSTR_PATH])


def gcc_instrument():
    """
    Instrument the build with GCC.
    """
    shutil.rmtree(GCC_INSTR_PATH, ignore_errors=True)
    prefix = shlex.quote(os.path.join(os.getcwd(), GCC_INSTR_PATH))
    gcov_data = shlex.quote(os.path.join(os.getcwd(), "gcov-data"))
    flags = f"-fprofile-generate={gcov_data} -fprofile-prefix-path={prefix}"
    run_command(
        [
            CMAKE,
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            GCC_INSTR_PATH,
            *GCC_CMAKE_OPTIONS,
            f"-DCMAKE_CXX_FLAGS={flags} {GCC_CXX_FLAGS}",
            f"-DCMAKE_C_FLAGS={flags} {GCC_CXX_FLAGS}",
        ]
    )
    run_command(["cmake", "--build", GCC_INSTR_PATH])


def clang_build(files):
    """
    Build with clang using the given profraw files.
    """
    profraw_files = collect_profraw_files(files)
    merged = os.getcwd() + os.sep + "merged.profdata"
    shutil.rmtree(merged, ignore_errors=True)
    run_command(
        [
            LLVM_PROFDATA,
            "merge",
            f"-output={merged}",
            *map(str, profraw_files),
        ]
    )
    shutil.rmtree(CLANG_BUILD_PATH, ignore_errors=True)
    run_command(
        [
            CMAKE,
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            CLANG_BUILD_PATH,
            *CLANG_CMAKE_OPTIONS,
            f"-DCMAKE_C_FLAGS=-fprofile-instr-use={shlex.quote(merged)} {CLANG_C_FLAGS}",
            f"-DCMAKE_CXX_FLAGS=-fprofile-instr-use={shlex.quote(merged)} {CLANG_CXX_FLAGS}",
        ]
    )
    run_command([CMAKE, "--build", CLANG_BUILD_PATH])


def perror(msg):
    """
    Print an error message and exit.
    """
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def gcc_build(files):
    """
    Merge and build with GCC using the given gcov profile data.
    """
    merged = os.path.join(os.getcwd(), "gcov-data")

    # shutil.rmtree(merged, ignore_errors=True)
    # gcov_files = []
    # for arg in files:
    #     p = Path(arg)
    #     if p.is_dir():
    #         gcov_files.extend(p.rglob("gcov-data"))
    #     else:
    #         perror(f"{arg} is not a valid directory.")
    # if not gcov_files:
    #     perror("No gcov-data directories found.")
    #
    # i = 0
    # lst = f"{merged}_{0}"
    # for file in gcov_files:
    #     cur = f"{merged}_{i}"
    #     if lst == cur:
    #         print(f"Copying {file} to {cur}")
    #         shutil.copytree(file, cur)
    #     else:
    #         print(f"Merging {lst} and {file} -> {cur}")
    #         run_command([GCOV_TOOL, "merge", "-o", cur, lst, str(file)])
    #         shutil.rmtree(lst, ignore_errors=True)
    #     lst = cur
    #     i += 1
    # shutil.move(lst, merged)

    shutil.rmtree(GCC_BUILD_PATH, ignore_errors=True)
    prefix = shlex.quote(os.path.join(os.getcwd(), GCC_BUILD_PATH))
    flags = f"-fprofile-use={shlex.quote(merged)} -fprofile-prefix-path={prefix}"
    run_command(
        [
            CMAKE,
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            GCC_BUILD_PATH,
            *GCC_CMAKE_OPTIONS,
            f"-DCMAKE_C_FLAGS={flags} {GCC_C_FLAGS}",
            f"-DCMAKE_CXX_FLAGS={flags} {GCC_CXX_FLAGS}",
        ]
    )
    run_command(["cmake", "--build", GCC_BUILD_PATH])


def main():
    """
    Run the PGO helper script.
    """
    parser = argparse.ArgumentParser(
        description="PGO helper for clang: instrument, run, and build with profile data."
    )
    parser.add_argument(
        "--compiler",
        choices=["clang", "gcc"],
        default="clang",
        help="Compiler to use (default: clang).",
    )
    subparsers = parser.add_subparsers(dest="mode", required=True)

    # instrument
    subparsers.add_parser(
        "instrument", help="Configure and build with instrumentation."
    )

    # build
    build_parser = subparsers.add_parser(
        "build", help="Merge and use profile data for final build."
    )
    build_parser.add_argument(
        "inputs", nargs="*", help="profraw file(s) or directory(ies) to search"
    )

    args = parser.parse_args()

    if args.mode == "instrument":
        if args.compiler == "gcc":
            gcc_instrument()
        else:
            clang_instrument()

    elif args.mode == "build":
        if args.compiler == "gcc":
            gcc_build(args.inputs)
        else:
            clang_build(args.inputs)


if __name__ == "__main__":
    main()
