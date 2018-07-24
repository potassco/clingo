#!/bin/zsh

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

set -e

cd "$(dirname $0)/.."

VERSION="$(grep '#define CLINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libclingo/clingo.h | colrm 1 23 | tr -d '"')"

# {{{1 parse options

clean=0

function usage() {
    cat <<EOF
Usage: $1 [-c] [-v <version>]
EOF
}

while getopts "hcv:" name; do
    case "${name}" in
        c)
            clean=1
            ;;
        v)
            VERSION="${OPTARG}"
            ;;
        h)
            usage $0
            exit 0
            ;;
        *)
            usage $0
            exit 1
            ;;
    esac
done

if [[ "$OPTIND" -le "${#}" ]]; then
    echo "$0: bad parameter: ${(P)OPTIND}"
    usage $0
    exit 1
fi

GRINGO="clingo-${VERSION}"
SRC="${GRINGO}-source"
branch="v${VERSION}"

set -ex

# {{{1 functions

function run_ssh() {
    ssh -T "${SSH_ARGS[@]}" "${@}"
}

function run_rsync() {
    rsync -ar "${RSYNC_ARGS[@]}" "${@}"
}

function copy_files() {
    for x in gringo clingo reify clasp lpconvert; do
        run_rsync "${2}:${TEMP}/build/bin/${EXTRA}${x}${EXT}" "${3}/${x}${EXT}"
        chmod +x "${3}/${x}${EXT}"
    done
    run_rsync "${SRC}/"{CHANGES.md,LICENSE.md,examples} "${3}"
    (setopt NULL_GLOB; rm -rf "${3}"/**/CMakeLists.txt)
    for x in c cc; do
        rm -rf "${3}"/**/"$x"
    done
    if [[ ${1} = "zip" ]]; then
        zip -r "${3}.zip" "${3}"
    else
        tar czf "${3}.tar.gz" "${3}"
    fi
}

function update_readme() {
    mkdir -p "$(dirname "$1")"
    sed '/INSTALL\.md/d' "${SRC}/README.md" > "$1"
    echo >> "$1"
    cat >> "$1"
}

function package() {

    if [[ $clean == 1 ]]; then
        run_ssh "${BUILD_HOST}" "rm -rf '${TEMP}'"
    fi

    run_ssh "${BUILD_HOST}" "mkdir -p '${TEMP}'"
    run_rsync "${SRC}/" "${BUILD_HOST}:${TEMP}/source"
    run_rsync "lua/" "${BUILD_HOST}:${TEMP}/lua"
    run_rsync "../scratch/lua/CMakeLists.txt" "${BUILD_HOST}:${TEMP}/lua/"

    run_ssh "${BUILD_HOST}" <<EOF
set -ex

function convert_path() {
    if [[ -x ~/local/bin/wslpath ]]; then
        ~/local/bin/wslpath -aw "\${1}"
    else
        b="\$(basename "\$1")"
        d="\$(dirname "\$1")"
        cd "\${d}"
        echo "\${PWD}/\${b}"
    fi
}

cd "${TEMP}"

"${CMAKE}" -E make_directory lua/build
"${CMAKE}" -E make_directory lua/install

"${CMAKE}" \\
    -H"\$(convert_path lua)" \\
    -B"\$(convert_path lua/build)" \\
    -G "${GENERATOR}" \\
    -DCMAKE_BUILD_TYPE=Release \\
    -DCMAKE_INSTALL_PREFIX="\$(convert_path lua/install)" ${EXTRA_FLAGS}
"${CMAKE}" --build "\$(convert_path lua/build)" --target install --config Release

"${CMAKE}" -E make_directory build
"${CMAKE}" \\
    -H"\$(convert_path source)" \\
    -B"\$(convert_path build)" \\
    -G "${GENERATOR}" \\
    -DCMAKE_BUILD_TYPE=Release \\
    -DCLINGO_BUILD_SHARED=OFF \\
    -DCLINGO_BUILD_WITH_PYTHON=OFF \\
    -DCLINGO_REQUIRE_LUA=ON \\
    -DCLINGO_BUILD_STATIC=ON \\
    -DCLINGO_MANAGE_RPATH=OFF \\
    -DLUA_LIBRARY="\$(convert_path lua/install/lib/*lua*)" \\
    -DLUA_INCLUDE_DIR="\$(convert_path lua/install/include)" ${EXTRA_FLAGS}
"${CMAKE}" --build "\$(convert_path build)" --config Release
EOF

    update_readme "${PACKAGE}/README.md" <<EOF
## Contents of ${PLATFORM} Binary Release

The \`clingo.exe\` and \`gringo.exe\` binaries are compiled with Lua 5.3 but
without Python support.

- \`clingo.exe\`: solver for non-ground programs
- \`gringo.exe\`: grounder
- \`clasp.exe\`: solver for ground programs
- \`reify.exe\`: reifier for ground programs
- \`lpconvert.exe\`: translator for ground formats
EOF
    copy_files "${ARCHIVE}" "${BUILD_HOST}" "${PACKAGE}"

}

# {{{1 preparation

if [[ $clean == 1 ]]; then
    rm -rf "release-${VERSION}"
fi

mkdir -p "release-${VERSION}"
cd "release-${VERSION}"

if [[ ! -e ${SRC} ]]; then
    git clone --branch "$branch" --single-branch --depth=1 git@github.com:potassco/clingo ${SRC}
    (cd ${SRC}; git submodule update --init --recursive)
    wget -c https://www.lua.org/ftp/lua-5.3.4.tar.gz
    tar -x --transform='s|^[^/]*|lua|' -f lua-5.3.4.tar.gz
    chmod -R u+w+r,g+r-w,o+r-w lua
    find lua -print0 | xargs -0 touch
fi

(setopt NULL_GLOB; rm -rf ${SRC}/{.ycm_extra_conf.py*,.travis.yml,.git*,scratch,TODO,Makefile})

# {{{1 Linux

EXTRA_FLAGS='-DCMAKE_EXE_LINKER_FLAGS="-pthread -static -s -Wl,-u,pthread_cond_broadcast,-u,pthread_cond_destroy,-u,pthread_cond_signal,-u,pthread_cond_timedwait,-u,pthread_cond_wait,-u,pthread_create,-u,pthread_detach,-u,pthread_equal,-u,pthread_getspecific,-u,pthread_join,-u,pthread_key_create,-u,pthread_key_delete,-u,pthread_mutex_lock,-u,pthread_mutex_unlock,-u,pthread_once,-u,pthread_setspecific"'
PACKAGE="${GRINGO}-linux-x86_64"
GENERATOR="Ninja"
PLATFORM="Linux"
ARCHIVE="tgz"
EXTRA=
EXT=
BUILD_HOST="ouessant"
TEMP="/tmp/${GRINGO}-work"
CMAKE='cmake'
SSH_ARGS=()
RSYNC_ARGS=()
package

# {{{1 Windows

PACKAGE="${GRINGO}-win64"
EXTRA_FLAGS='-DCMAKE_CXX_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG" -DCMAKE_C_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG"'
GENERATOR="Visual Studio 14 2015 Win64"
PLATFORM="Windows"
ARCHIVE="zip"
EXTRA="Release/"
EXT=".exe"
BUILD_HOST="localhost"
# the linux tmp will confuse cmake; this has to be a folder in the windows world
TEMP="/home/kaminski/git/tmp/${GRINGO}-work"
CMAKE='/mnt/c/Program Files (x86)/Microsoft Visual Studio/2017/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
SSH_ARGS=("-p" "2222")
RSYNC_ARGS=("-e" "ssh -p 2222")
package

# {{{1 MacOS

PACKAGE="${GRINGO}-macos-x86_64"
EXTRA_FLAGS=""
GENERATOR="Ninja"
PLATFORM="MacOS"
ARCHIVE="tgz"
EXTRA=""
EXT=""
BUILD_HOST="kaminski_local@herm"
TEMP="/tmp/${GRINGO}-work"
CMAKE='cmake'
SSH_ARGS=()
RSYNC_ARGS=()
package
