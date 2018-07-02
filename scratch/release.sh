#!/bin/zsh

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

set -e

cd "$(dirname $0)/.."

VERSION="$(grep '#define CLINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libclingo/clingo.h | colrm 1 23 | tr -d '"')"
GRINGO="clingo-${VERSION}"
GRINGO_MAC="${GRINGO}-macos-10.12"
GRINGO_WIN64="${GRINGO}-win64"
GRINGO_LIN64="${GRINGO}-linux-x86_64"
SRC="${GRINGO}-source"
TEMP="/tmp/${GRINGO}-work"
MAC=kaminski_local@herm
WIN64=rhuys
LIN64=ouessant
RSYNC=( )
EXTRA=
EXT=
SSH=ssh

# {{{1 setup

clean=0
branch="v${VERSION}"

function usage() {
    cat <<EOF
Usage: $1 [-c] [-b <branch>]
EOF
}

while getopts "hcb:" name; do
    case "${name}" in
        c)
            clean=1
            ;;
        b)
            branch="${OPTARG}"
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

set -ex

if [[ $clean == 1 ]]; then
    rm -rf "release-${VERSION}"
    ssh -T "${MAC}" "rm -rf '${TEMP}'"
    ssh -T "${LIN64}" "rm -rf '${TEMP}'"
fi

mkdir -p "release-${VERSION}"
cd "release-${VERSION}"

if [[ ! -e ${SRC} ]]; then
    git clone --branch "$branch" --single-branch --depth=1 git@github.com:potassco/clingo ${SRC}
    (cd ${SRC}; git submodule update --init --recursive)
    wget -c https://www.lua.org/ftp/lua-5.3.4.tar.gz
    tar -x --transform='s|^[^/]*|lua|' -f lua-5.3.4.tar.gz
    sed -i 's/^CFLAGS= -O2/CFLAGS= -O3 -DNDEBUG/' 'lua/src/Makefile'
    chmod -R u+w+r,g+r-w,o+r-w lua
    find lua -print0 | xargs -0 touch
fi

(setopt NULL_GLOB; rm -rf ${SRC}/{.ycm_extra_conf.py*,.travis.yml,.git*,scratch,TODO,Makefile})

function copy_files() {
    for x in gringo clingo reify clasp lpconvert; do
        scp "${@:4}" -p "${2}:${TEMP}/build/bin/${EXTRA}${x}${EXT}" "${3}/${x}${EXT}"
        chmod +x "${3}/${x}${EXT}"
    done
    scp "${@:4}" -rp "${SRC}/"{CHANGES.md,LICENSE.md,examples} "${3}"
    (setopt NULL_GLOB; rm -rf "${3}"/**/CMakeLists.txt)
    for x in "${RSYNC[@]}"; do
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

# {{{1 build for linux x86_64

ssh -T "${LIN64}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${LIN64}:${TEMP}/source"
rsync -ar "lua/" "${LIN64}:${TEMP}/lua"

ssh -T "${LIN64}" <<EOF
set -ex

cd "${TEMP}/lua"
make -j8 generic CC=cc
make local

mkdir -p "${TEMP}/build"
cd "${TEMP}/build"

cmake "${TEMP}/source" -DLUA_INCLUDE_DIR=${TEMP}/lua/install/include -DLUA_LIBRARY=${TEMP}/lua/install/lib/liblua.a -DCLINGO_REQUIRE_LUA=On -DCLINGO_BUILD_WITH_LUA=ON -DCLINGO_BUILD_WITH_PYTHON=OFF -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=release -DCLINGO_BUILD_STATIC=ON -DCLINGO_MANAGE_RPATH=Off -DCMAKE_EXE_LINKER_FLAGS="-pthread -static -s -Wl,-u,pthread_cond_broadcast,-u,pthread_cond_destroy,-u,pthread_cond_signal,-u,pthread_cond_timedwait,-u,pthread_cond_wait,-u,pthread_create,-u,pthread_detach,-u,pthread_equal,-u,pthread_getspecific,-u,pthread_join,-u,pthread_key_create,-u,pthread_key_delete,-u,pthread_mutex_lock,-u,pthread_mutex_unlock,-u,pthread_once,-u,pthread_setspecific"
make -j8 VERBOSE=1
EOF

update_readme "${GRINGO_LIN64}/README.md" <<"EOF"
## Contents of Linux Binary Release

The `clingo` and `gringo` binaries are compiled statically and include Lua 5.3
but no Python support.

- `clingo`: solver for non-ground programs
- `gringo`: grounder
- `clasp`: solver for ground programs
- `reify`: reifier for ground programs
- `lpconvert`: translator for ground formats
EOF
RSYNC=( cc c ) copy_files tgz "${LIN64}" "${GRINGO_LIN64}"

# {{{1 build for macos

ssh -T "${MAC}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${MAC}:${TEMP}/source"
rsync -ar "lua/" "${MAC}:${TEMP}/lua"

ssh -T "${MAC}" <<EOF
set -ex

cd "${TEMP}/lua"
make -j8 generic CC=cc
make local

COMMON=(-DLUA_INCLUDE_DIR=${TEMP}/lua/install/include -DLUA_LIBRARY=${TEMP}/lua/install/lib/liblua.a -DCMAKE_BUILD_TYPE=release -DCLINGO_MANAGE_RPATH=Off -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison -DPYTHON_EXECUTABLE=/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python)

(
mkdir -p "${TEMP}/build"
cd "${TEMP}/build"

cmake "${TEMP}/source" -DCLINGO_BUILD_WITH_LUA=ON -DCLINGO_REQUIRE_LUA=ON -DCLINGO_BUILD_WITH_PYTHON=OFF -DCLINGO_BUILD_SHARED=OFF "\${COMMON[@]}"
make -j8 VERBOSE=1
)
EOF

(
update_readme "${GRINGO_MAC}/README.md" <<"EOF"
## Contents of MacOS Binary Release

The `clingo` and `gringo` binaries are compiled with Lua 5.3 but without Python
support.

- `clingo`: solver for non-ground programs
- `gringo`: grounder
- `clasp`: solver for ground programs
- `reify`: reifier for ground programs
- `lpconvert`: translator for ground formats
EOF
copy_files tgz "${MAC}" "${GRINGO_MAC}"
)

# {{{1 build for win64

exit 0

# NOTE: requires cmake, python, visual studio, and a cygwin environment with re2c to be installed on build machine

# for some incomprensible reason ssh must run in an interactive shell
function win_ssh() {
    ssh -p 2264 "${@}"
}

function win_rsync() {
    rsync -e 'ssh -p 2264' "${@}"
}

CMAKE="TEMP='C:\\TEMP' TMP='C:\\TEMP' /cygdrive/c/Program\ Files/CMake/bin/cmake.exe"

if [[ $1 == clean ]]; then
    echo "rm -rf '${TEMP}'" | win_ssh -T "${WIN64}"
fi

echo "mkdir -p '${TEMP}'" | win_ssh -T "${WIN64}"
win_rsync -ar "${SRC}/" "${WIN64}:${TEMP}/source"
win_rsync -ar "lua/" "${WIN64}:${TEMP}/lua"

win_ssh -T "${WIN64}" <<EOF
set -ex
mkdir -p '${TEMP}'

cd '${TEMP}'

BISON_EXECUTABLE="\$(cygpath -w \$(which bison))"
RE2C_EXECUTABLE="\$(cygpath -w \$(which re2c))"
SOURCE="\$(cygpath -w ${TEMP}/source)"

cat > lua/CMakeLists.txt <<"EOS"
cmake_minimum_required (VERSION 2.6)
project (lua)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CORE
    src/lapi.c src/lcode.c src/lctype.c src/ldebug.c src/ldo.c src/ldump.c src/lfunc.c src/lgc.c src/llex.c
    src/lmem.c src/lobject.c src/lopcodes.c src/lparser.c src/lstate.c src/lstring.c src/ltable.c
    src/ltm.c src/lundump.c src/lvm.c src/lzio.c)
set(LIB
    src/lauxlib.c src/lbaselib.c src/lbitlib.c src/lcorolib.c src/ldblib.c src/liolib.c
    src/lmathlib.c src/loslib.c src/lstrlib.c src/ltablib.c src/lutf8lib.c src/loadlib.c src/linit.c)

add_library(lua STATIC \${CORE} \${LIB})
target_include_directories(lua PUBLIC src)

install(TARGETS lua
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(FILES
    "src/lua.h"
    "src/luaconf.h"
    "src/lualib.h"
    "src/lauxlib.h"
    "src/lua.hpp"
    DESTINATION include)
EOS

# build lua
cd '${TEMP}'
cd lua
mkdir -p install
${CMAKE} -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=install
${CMAKE} --build . --target install --config Release
export LUA_DIR=\$(cygpath -w ${TEMP}/lua/install)
echo "\${LUA_DIR}"

# build clingo
mkdir -p "${TEMP}/build"
cd '${TEMP}/build'
${CMAKE} -G "Visual Studio 14 2015 Win64" -DCLINGO_BUILD_SHARED=OFF -DCLINGO_BUILD_WITH_PYTHON=OFF -DBISON_EXECUTABLE="\${BISON_EXECUTABLE}" -DRE2C_EXECUTABLE="\${RE2C_EXECUTABLE}" -DCLINGO_REQUIRE_LUA=ON "\${SOURCE}"
${CMAKE} --build . --config Release
EOF

(
EXTRA=Release/
EXT=.exe
SSH=win_ssh
update_readme "${GRINGO_WIN64}/README.md" <<"EOF"
## Contents of Windows Binary Release

The `clingo.exe` and `gringo.exe` binaries are compiled with Lua 5.3 but
without Python support.

- `clingo.exe`: solver for non-ground programs
- `gringo.exe`: grounder
- `clasp.exe`: solver for ground programs
- `reify.exe`: reifier for ground programs
- `lpconvert.exe`: translator for ground formats
EOF
copy_files zip "${WIN64}" "${GRINGO_WIN64}" -P 2264
)
