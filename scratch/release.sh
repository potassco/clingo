#!/bin/zsh

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

set -ex

cd "$(dirname $0)/.."

VERSION="$(grep '#define CLINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libclingo/clingo.h | colrm 1 23 | tr -d '"')"
GRINGO="clingo-${VERSION}"
GRINGO_MAC="${GRINGO}-macos-10.9"
GRINGO_WIN64="${GRINGO}-win64"
GRINGO_LIN64="${GRINGO}-linux-x86_64"
SRC="${GRINGO}-source"
TEMP="/tmp/${GRINGO}-work"
MAC=kaminski_local@herm
WIN64=rhuys
LIN64=zuse
RSYNC=( )
EXTRA=
EXT=
PYEXT=.so
LIBSUF=lib
LIBEXT=.so
SSH=ssh

# {{{1 setup

if [[ $1 == clean ]]; then
    rm -rf "release-${VERSION}"
    ssh -T "${MAC}" "rm -rf '${TEMP}'"
    ssh -T "${LIN64}" "rm -rf '${TEMP}'"
fi

mkdir -p "release-${VERSION}"
cd "release-${VERSION}"

if [[ ! -e ${SRC} ]]; then
    #git clone --branch "v${VERSION}" --single-branch --depth=1 git@github.com:potassco/clingo ${SRC}
    git clone --branch wip --single-branch --depth=1 git@github.com:potassco/clingo ${SRC}
    (cd ${SRC}; git submodule update --init --recursive)
    wget -c https://www.lua.org/ftp/lua-5.3.4.tar.gz
    tar -x --transform='s|^[^/]*|lua|' -f lua-5.3.4.tar.gz
    sed -i 's/^CFLAGS= -O2/CFLAGS= -O3 -DNDEBUG/' 'lua/src/Makefile'
    chmod -R u+w+r,g+r-w,o+r-w lua
    find lua -print0 | xargs -0 touch
fi

(setopt NULL_GLOB; rm -rf ${SRC}/{.ycm_extra_conf.py*,.travis.yml,.git*,scratch,TODO,Makefile})

function copy_files() {
    mkdir -p "${3}"
    for x in gringo clingo reify clasp lpconvert; do
        scp -p "${2}:${TEMP}/build/bin/${EXTRA}${x}${EXT}" "${3}/${x}${EXT}"
        chmod +x "${3}/${x}${EXT}"
    done
    if echo "test -d '${TEMP}/python'" | ${SSH} ${2}; then
        for x in gringo clingo; do
            scp -p "${2}:${TEMP}/python/bin/${EXTRA}${x}${EXT}" "${3}/${x}-python${EXT}"
            chmod +x "${3}/${x}-python${EXT}"
        done
        mkdir -p ${3}/python-api
        scp -p "${2}:${TEMP}/python/bin/python/${EXTRA}clingo${PYEXT}" "${3}/python-api/"
        chmod +x "${3}/python-api/clingo${PYEXT}"
    fi
    if echo "test -d '${TEMP}/c-api'" | ${SSH} ${2}; then
        mkdir -p ${3}/c-api/{lib,include}
        scp -p "${2}:${TEMP}/c-api/bin/${EXTRA}${LIBSUF}clingo${LIBEXT}" "${3}/c-api/lib/"
        chmod +x "${3}/c-api/lib/${LIBSUF}clingo${LIBEXT}"
        scp -p "${2}:${TEMP}/source/libclingo/"{clingo.h,clingo.hh} "${3}/c-api/include/"
    fi
    scp -rp "${SRC}/"{CHANGES.md,LICENSE.md,README.md,examples} "${3}"
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
    sed -i '/INSTALL\.md/d' "$1"
    echo >> "$1"
    cat >> "$1"
}

# {{{1 build for linux x86_64

ssh -T "${LIN64}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${LIN64}:${TEMP}/source"
rsync -ar "lua/" "${LIN64}:${TEMP}/lua"

ssh -T "${LIN64}" <<EOF
set -ex

module load cmake gcc/4.9.1 bison re2c python/2.7

cd "${TEMP}/lua"
make -j8 posix CC=cc
make local

mkdir -p "${TEMP}/build"
cd "${TEMP}/build"

export LUA_DIR="${TEMP}/lua/install"
cmake "${TEMP}/source" -DCLINGO_REQUIRE_LUA=On -DCLINGO_BUILD_WITH_LUA=ON -DCLINGO_BUILD_WITH_PYTHON=OFF -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=release -DCLINGO_BUILD_STATIC=ON -DCLINGO_MANAGE_RPATH=Off -DCMAKE_EXE_LINKER_FLAGS="-pthread -static -s -Wl,-u,pthread_cancel,-u,pthread_cond_broadcast,-u,pthread_cond_destroy,-u,pthread_cond_signal,-u,pthread_cond_timedwait,-u,pthread_cond_wait,-u,pthread_create,-u,pthread_detach,-u,pthread_join,-u,pthread_equal"
make -j8 VERBOSE=1
EOF

RSYNC=( cc c ) copy_files tgz "${LIN64}" "${GRINGO_LIN64}"
update_readme "${GRINGO_LIN64}/README.md" <<"EOF"
## Contents of Linux Binary Release

The `clingo` and `gringo` binaries are compiled statically and include Lua 5.3
but no Python support. For Python support please get a source release and
compile clingo yourself.

- `clingo`: solver for non-ground programs
- `gringo`: grounder
- `clasp`: solver for ground programs
- `reify`: reifier for ground programs
- `lpconvert`: translator for ground formats
EOF

# {{{1 build for macos

ssh -T "${MAC}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${MAC}:${TEMP}/source"
rsync -ar "lua/" "${MAC}:${TEMP}/lua"

ssh -T "${MAC}" <<EOF
set -ex

cd "${TEMP}/lua"
make -j8 posix CC=cc
make local

export LUA_DIR="${TEMP}/lua/install"
COMMON=(-DCMAKE_BUILD_TYPE=release -DCLINGO_MANAGE_RPATH=Off -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison -DLUA_LIBRARY:FILEPATH="\${LUA_DIR}/lib/liblua.a" -DPYTHON_EXECUTABLE=/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python)

(
mkdir -p "${TEMP}/build"
cd "${TEMP}/build"

cmake "${TEMP}/source" -DCLINGO_BUILD_WITH_LUA=ON -DCLINGO_REQUIRE_LUA=ON -DCLINGO_BUILD_WITH_PYTHON=OFF -DCLINGO_BUILD_SHARED=OFF "\${COMMON[@]}"
make -j8 VERBOSE=1
)

(
mkdir -p "${TEMP}/python"
cd "${TEMP}/python"

cmake "${TEMP}/source" -DCLINGO_REQUIRE_LUA=ON -DCLINGO_REQUIRE_PYTHON=ON -DCLINGO_BUILD_SHARED=OFF "\${COMMON[@]}"
make -j8 VERBOSE=1 gringo clingo pyclingo
)

(
mkdir -p "${TEMP}/c-api"
cd "${TEMP}/c-api"
cmake "${TEMP}/source" -DCLINGO_BUILD_WITH_LUA=OFF -DCLINGO_BUILD_WITH_PYTHON=OFF -DCLINGO_BUILD_SHARED=ON "\${COMMON[@]}"
make -j8 VERBOSE=1 libclingo
)
EOF

(
LIBEXT=.dylib
copy_files tgz "${MAC}" "${GRINGO_MAC}"
update_readme "${GRINGO_MAC}/README.md" <<"EOF"
## Contents of MacOS Binary Release

The `clingo` and `gringo` binaries are compiled with Lua 5.3 but without Python
2.7 support.  The `clingo-python` and `gringo-python` executables are
additionally build with Python 2.7 support.

- `clingo`: solver for non-ground programs
- `clingo-python`: solver for non-ground programs with Python support
- `gringo`: grounder
- `gringo-python`: grounder with Python support
- `clasp`: solver for ground programs
- `reify`: reifier for ground programs
- `lpconvert`: translator for ground formats
- `c-api/`: headers and library of clingo C and C++ API
- `python-api/`: clingo Python 2.7 module
  - to use the module either copy it into the Python path or point the
    [PYTHONPATH](https://docs.python.org/2/using/cmdline.html#envvar-PYTHONPATH)
    to the `python-api` directory
EOF
)

# {{{1 build for win64

# NOTE: requires cmake, python, git bash, and ssh server to be installed on build machine

# for some incomprensible reason ssh must run in an interactive shell
function win_ssh() {
    script --return -c '/usr/bin/ssh -xT "'"${1}"'" -- "\"C:\Program Files\Git\bin\bash.exe\""' /dev/null
}

TEMP="C:/Users/kaminski/${GRINGO}-work"
if [[ $1 == clean ]]; then
    echo "rm -rf '${TEMP}'" | win_ssh "${WIN64}"
fi

echo "mkdir -p '${TEMP}'" | win_ssh "${WIN64}"
scp -pr "${SRC}/." "${WIN64}:${TEMP}/source"
scp -pr "lua/." "${WIN64}:${TEMP}/lua"

win_ssh "${WIN64}" <<EOF
set -ex
mkdir -p '${TEMP}'

cd '${TEMP}'

[[ -e re2c.exe ]] || curl -LO http://cs.uni-potsdam.de/~kaminski/re2c.exe
[[ -e win_flex_bison-latest.zip ]] || curl -LO http://cs.uni-potsdam.de/~kaminski/win_flex_bison-latest.zip

unzip -ud bison win_flex_bison-latest.zip

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

EOF

# the loops are workarounds

# build lua
for i in a b; do
win_ssh "${WIN64}" <<EOF
set -ex
cd '${TEMP}'
cd lua
mkdir -p install
cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=install
cmake --build . --target install --config Release
EOF
done

# build clingo without python
for i in a b; do
win_ssh "${WIN64}" <<EOF
set -ex
mkdir -p "${TEMP}/build"
cd '${TEMP}/build'
export LUA_DIR=\$(cmd //c echo ${TEMP}/lua/install)
echo "\${LUA_DIR}"
cmake -G "Visual Studio 14 2015 Win64" -DCLINGO_BUILD_SHARED=OFF -DCLINGO_BUILD_WITH_PYTHON=OFF -DBISON_EXECUTABLE="${TEMP}/bison/win_bison.exe" -DRE2C_EXECUTABLE="${TEMP}/re2c.exe" -DCLINGO_REQUIRE_LUA=ON "${TEMP}/source"
cmake --build . --config Release
EOF
done

# build clingo api
for i in a b; do
win_ssh "${WIN64}" <<EOF
set -ex
mkdir -p "${TEMP}/c-api"
cd '${TEMP}/c-api'
export LUA_DIR=\$(cmd //c echo ${TEMP}/lua/install)
echo "\${LUA_DIR}"
cmake -G "Visual Studio 14 2015 Win64" -DCLINGO_BUILD_SHARED=ON -DCLINGO_BUILD_WITH_LUA=OFF -DCLINGO_BUILD_WITH_PYTHON=OFF -DBISON_EXECUTABLE="${TEMP}/bison/win_bison.exe" -DRE2C_EXECUTABLE="${TEMP}/re2c.exe" "${TEMP}/source"
cmake --build . --target libclingo --config Release
EOF
done

# build clingo with python
for i in a b; do
win_ssh "${WIN64}" <<EOF
set -ex
mkdir -p "${TEMP}/python"
cd '${TEMP}/python'
export LUA_DIR=\$(cmd //c echo ${TEMP}/lua/install)
echo "\${LUA_DIR}"
cmake -G "Visual Studio 14 2015 Win64" -DCLINGO_BUILD_SHARED=OFF -DCLINGO_REQUIRE_LUA=ON -DCLINGO_BUILD_WITH_PYTHON=ON -DCLINGO_REQUIRE_PYTHON=ON -DBISON_EXECUTABLE="${TEMP}/bison/win_bison.exe" -DRE2C_EXECUTABLE="${TEMP}/re2c.exe" -DPYTHON_EXECUTABLE="c:/program files/python27/python.exe" "${TEMP}/source"
cmake --build . --target clingo --config Release
cmake --build . --target gringo --config Release
cmake --build . --target pyclingo --config Release
EOF
done
(
EXTRA=Release/
EXT=.exe
SSH=win_ssh
PYEXT=.pyd
LIBSUF=
LIBEXT=.dll
copy_files zip "${WIN64}" "${GRINGO_WIN64}"
update_readme "${GRINGO_WIN64}/README.md" <<"EOF"
## Contents of Windows Binary Release

The `clingo.exe` and `gringo.exe` binaries are compiled with Lua 5.3 but
without Python support. The `clingo-python.exe` and `gringo-python.exe`
executables are additionally build with Python 2.7 support.

To run the executables, you may have to install the [Visual C++ Redistributable
for Visual Studio 2015](
https://www.microsoft.com/en-us/download/details.aspx?id=48145). When
downloading choose the `vc_redist.x64.exe` executable.

To run the executables with Python support, you have to install [Python 2.7.13
for Windows x86-64](
https://www.python.org/ftp/python/2.7.13/python-2.7.13.amd64.msi).

- `clingo.exe`: solver for non-ground programs
- `clingo-python.exe`: solver for non-ground programs with Python support
- `gringo.exe`: grounder
- `gringo-python.exe`: grounder with Python support
- `clasp.exe`: solver for ground programs
- `reify.exe`: reifier for ground programs
- `lpconvert.exe`: translator for ground formats
- `c-api/`: headers and library of clingo C and C++ API
- `python-api/`: clingo Python 2.7 module
  - to use the module either copy it into the Python path or point the
    [PYTHONPATH](https://docs.python.org/2/using/cmdline.html#envvar-PYTHONPATH)
    to the `python-api` directory
EOF
)
