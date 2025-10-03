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
        # gcc
        make release
        make release_lto
        ./scripts/pgo.py --compiler gcc instrument
        # clang
        make release_clang
        make release_clang_lto
        ./scripts/pgo.py --compiler clang instrument
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
        ./scripts/pgo.py --compiler gcc build ./bench/output
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
