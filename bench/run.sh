#!/bin/bash

function usage() {
    echo "run {prepare,profile,build,benchmark,eval,clean}"
    exit 1
}

[[ "$#" -eq 1 ]] || usage

set -e

cd "$(dirname "$(realpath "$0")")"

function cps() {
    # NOTE: probably too complicated but the ln docs are hard to grasp
    source="$(realpath "$1")"
    target="$(dirname "$2")"
    name="$(basename "$2")"
    (
        cd "${target}"
        ln -rsf "${source}" "${name}"
    )
}

case "$1" in
prepare)
    (
        cd ..
        # gcc
        make release
        cps build/release/bin/clingo bench/programs/clingo-6.0.0
        make release_lto
        cps build/release_lto/bin/clingo bench/programs/clingo-lto-6.0.0
        ./scripts/pgo.py --compiler gcc instrument
        cps build/release_instrument/bin/clingo bench/programs/clingo-instrument-6.0.0
        # clang
        make release_clang
        cps build/release_clang/bin/clingo bench/programs/clingo-clang-6.0.0
        make release_clang_lto
        cps build/release_clang_lto/bin/clingo bench/programs/clingo-clang-lto-6.0.0
        ./scripts/pgo.py --compiler clang instrument
        cps build/release_clang_instrument/bin/clingo bench/programs/clingo-clang-instrument-6.0.0
    )
    bgen ./runscripts/local.xml
    ;;
profile)
    ./output/clingo-instrument/precision-3480/start.py
    ;;
build)
    (
        cd ..
        ./scripts/pgo.py --compiler clang build ./bench/output
        cps build/release_clang_pgo/bin/clingo bench/programs/clingo-clang-pgo-6.0.0
        ./scripts/pgo.py --compiler gcc build ./bench/output
        cps build/release_pgo/bin/clingo bench/programs/clingo-pgo-6.0.0
    )
    ;;
benchmark)
    ./output/clingo-benchmark/precision-3480/start.py
    ;;
eval)
    beval ./runscripts/local.xml >results/results.xml
    bconv -p clingo-benchmark results/results.xml -o results/results.ods
    ;;
clean)
    rm -rf ./output
    ;;
*)
    usage
    ;;
esac
