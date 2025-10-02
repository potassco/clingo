#!/bin/bash

function usage() {
    echo "run {prepare,profile,build,benchmark,eval,clean}"
    exit 1
}

[[ "$#" -eq 1 ]] || usage

set -e

cd "$(dirname "$(realpath "$0")")"

case "$1" in
prepare)
    (
        cd ..
        make release_clang
        cp -fs build/relase_clang/bin/clingo bench/programs/clingo-clang-6.0.0
        make release_clang_lto
        cp -fs build/release_clang_lto/bin/clingo bench/programs/clingo-clang-lto-6.0.0
        ./scripts/pgo-clang.py instrument
        cp -fs build/relase_clang_instrument/bin/clingo bench/programs/_clingo-clang-instrument-6.0.0
    )
    bgen ./runscripts/local.xml
    ;;
profile)
    ./output/clingo-instrument/precision-3480/start.py
    ;;
build)
    (
        cd ..
        ./scripts/pgo-clang.py build ./bench/output
        cp -fs build/release_clang_pgo/bin/clingo bench/programs/clingo-clang-pgo-6.0.0
    )
    ;;
benchmark)
    ./output/clingo-benchmark/precision-3480/start.py
    ;;
eval)
    beval ./runscripts/local.xml >results/results.xml
    bconv results/results.xml -o results/results.ods
    ;;
clean)
    rm -rf ./output
    ;;
*)
    usage
    ;;
esac
