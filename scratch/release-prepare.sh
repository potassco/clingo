#!/bin/zsh
set -ex

cd "$(dirname "$0")/.."

git checkout master
git pull
git submodule update --init --recursive

version="$(sed -n 's/#define CLINGO_VERSION "\([0-9]\.[0-9]\.[0-9]\)"/\1/p' libclingo/clingo.h)"
dir="release-${version}"
message="prepare release for clingo-$version"
gitref="$(git show-ref refs/heads/master | sed 's/[ ].*//')"
submodules=(${(f)"$(cat .gitmodules | sed -n 's/\[submodule "\([^"]*\)"\]/\1/p')"})

function sync() {
    rsync -a --exclude '.git' --exclude '.gitmodules' "../$1/" "$1/"
    git add "$1"
}

git worktree add "${dir}" ${gitref}
(
    cd $dir

    for submodule in "${submodules[@]}"; do
        git rm "${submodule}"
    done
    git rm TODO.md
    git rm -r scratch
    git commit -m "${message}"

    for submodule in "${submodules[@]}"; do
        sync "${submodule}"
    done

    git commit --amend -m "${message}"
    git tag "v${version}"
)
