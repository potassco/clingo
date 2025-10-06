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
gather)
    IFS=$'\n' read -r -d '' -a runscript < <(printf '%s\0' "$(cat runscripts/local.xml)")
    rm runscripts/local.xml
    for line in "${runscript[@]}"; do
        if [[ $line =~ .*'<benchmark name="'(.*)'">'.* ]]; then
            echo "${line}" >>runscripts/local.xml
            name="${BASH_REMATCH[1]}"
            if [[ $name = clingo-instrument ]]; then
                source=instances.easy
            elif [[ $name = clingo ]]; then
                source=instances.competition
            else
                continue
            fi
            find . -name "${source}" | while read -r file; do
                loc="$(dirname "$file")"
                class="${loc#*/*/}"
                echo "        <files path=\"benchmarks/${class}\">" >>runscripts/local.xml
                echo "            <encoding file=\"benchmarks/${class}/encoding.lp\"/>" >>runscripts/local.xml
                cat "${file}" | while read -r instance; do
                    if [[ ! -e benchmarks/${class}/${instance} ]]; then
                        echo "Instance benchmarks/${class}/${instance} not found, skipping" >&2
                        continue
                    fi
                    echo "            <add file=\"${instance}\"/>" >>runscripts/local.xml
                done
                echo '        </files>' >>runscripts/local.xml
            done
            inside=1
            continue
        fi
        if [[ $line == *'</benchmark>'* && $inside -eq 1 ]]; then
            echo "${line}" >>runscripts/local.xml
            inside=0
            continue
        fi
        if [[ $inside -eq 0 ]]; then
            echo "${line}" >>runscripts/local.xml
            continue
        fi
    done
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
