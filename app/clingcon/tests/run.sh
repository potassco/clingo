#!/bin/zsh

# scons unsets this
export LC_ALL=C

function normalize() {
    next=0
    current=( )
    step=0
    result="ERROR"
    while read line; do
        if [[ next -eq 1 ]]; then
            if [[ step -gt 0 ]]; then
                print "Step: $step"
                current=(${(@on)current})
                for answer in "${current[@]}"; do
                    print $answer
                done
                current=( )
            fi
            step=$[step+1]
            next=0
        fi
        if [[ next -eq 2 ]]; then
            answer=(${(s: :)line})
            answer=(${(@on)answer})
            current+=("${answer[*]}")
            next=0
        elif [[ "$line" =~ "^Solving..." ]]; then
            next=1
        elif [[ "$line" =~ "^Answer: " ]]; then
            next=2
        elif [[ "$line" =~ "^SATISFIABLE" ]]; then
            result="SAT"
            next=1
        elif [[ "$line" =~ "^UNSATISFIABLE" ]]; then
            result="UNSAT"
            next=1
        elif [[ "$line" =~ "^UNKNOWN" ]]; then
            result="UNKNOWN"
            next=1
        elif [[ "$line" =~ "^OPTIMUM FOUND" ]]; then
            result="OPTIMUM FOUND"
            next=1
        fi
    done
    print "$result"
}

function usage() {
    cat << EOF
Usage:
  run.sh {-h,--help,help}
  run.sh [PATH-TO-CLINGO] [-- CLINGO-OPTIONS]
  run.sh normalize FILE [PATH-TO-CLINGO] [-- CLINGO-OPTIONS]

The first invocation prints this help, the second runs all tests, and the third
takes a logic program runs clingo and normalizes its output.
EOF
}

if [[ $# > 0 && ( "$1" == "--help" || $1 == "-h" || $1 == "help" ) ]]; then
    usage
    exit 0
fi
wd=$(cd "$(dirname "$0")"; pwd)
norm=0
if [[ $# > 0 && "${1}" == "normalize" ]]; then
    norm=1
    shift
    if [[ $# == 0 ]]; then 
        usage
        exit 1
    fi
    file="$1"
    shift
fi
if [[ $# == 0 || "$1" == "--" ]]; then
    clingo="$wd/../../../build/debug/clingo"
else
    clingo="$1"
    shift
fi
if [[ $# > 0 && "$1" != "--" ]] then
    usage
    exit 1
fi
[[ $# > 0 && "$1" == "--" ]] && shift
if [[ $norm == 1 ]]; then
    name=${file%.lp}
    opts=( )
    if [[ -e "$name.cmd" ]]; then
        opts=$(cat "$name.cmd")
        opts=(${(s: :)opts})
    fi
    $clingo 0 "${opts[@]}" "$file" "$@" | normalize
    exit 0
else
    run=0
    fail=0
    failures=()
    for x in $wd/**/*.lp; do
        run=$[run+1]
        name=${x%.lp}
        opts=( )
        if [[ -e "$name.cmd" ]]; then
            opts=$(cat "$name.cmd")
            opts=(${(s: :)opts})
        fi
        if $clingo 0 $x -Wno-operation-undefined -Wno-atom-undefined "${opts[@]}" "$@" | normalize | diff - "$name.sol"; then
            print -n "."
        else
            print -n "F"
            fail=$[fail+1]
            failures+=($x)
            #TODO: record failure and report at the end
        fi
    done
    print
    print
    print -n "OK ($[run-fail]/${run})"
    print
    print
    if [[ fail -gt 0 ]]; then
        print "The following tests failed:"
        for x in "${failures[@]}"; do
            print "  $x"
        done
    fi
    [[ fail -eq 0 ]]
fi
