#!/bin/bash

function usage() {
    echo "Usage: $0 file1 [file2 ...] -- command [args...]"
    exit 1
}

function is_same() {
    [[ "$(stat -c %i "$1")" == "$(stat -c %i "$2")" ]]
}

files=()
while [[ "$1" != "--" && $# -gt 0 ]]; do
    files+=("$1")
    shift
done

if [[ "$1" == "--" ]]; then
    shift
else
    usage
fi

if [[ $# -eq 0 ]]; then
    usage
fi

cmd=("$@")

dirs=()
for f in "${files[@]}"; do
    d=$(dirname "$f")
    [[ " ${dirs[*]} " == *" $d "* ]] || dirs+=("$d")
done

inotifywait -m -e close_write "${dirs[@]}" |
    while read -r dir event file; do
        for f in "${files[@]}"; do
            if is_same "$dir/$file" "$f" && [[ "$event" = "CLOSE_WRITE,CLOSE" ]]; then
                printf "\033c"
                "${cmd[@]}"
                break
            fi
        done
    done
