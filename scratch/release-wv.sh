#!/bin/bash
set -xe

VERSION="$(grep '#define CLINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libclingo/clingo.h | colrm 1 23 | tr -d '"')"
MINOR=${VERSION%.*}
MAJOR=${MINOR%.*}

cd "$(dirname $0)/.."

prefix=/home/wv/opt/clingo-${VERSION}
source="$(pwd -P)"

mkdir -p ${prefix}

mkdir -p build/${VERSION}
(
    cd build/${VERSION}
    cd "$(pwd -P)"
    cmake "${source}" \
        -DCMAKE_BUILD_TYPE=release \
        -DCMAKE_VERBOSE_MAKEFILE=On \
        -DCLINGO_BUILD_TESTS=On \
        -DCLASP_BUILD_TESTS=On \
        -DLIB_POTASSCO_BUILD_TESTS=On \
        -DCLINGO_BUILD_EXAMPLES=On \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DPYCLINGO_INSTALL_DIR="${prefix}/lib/python/2.7" \
        -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.1" \
        -DLUA_INCLUDE_DIR="/usr/include/lua5.1" \
        -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.1.so" \
        -DCMAKE_EXE_LINKER_FLAGS="-s" \
        -DCMAKE_SHARED_LINKER_FLAGS="-s"
    make -j8
    make test
    make install
)

cd /home/wv/bin/linux/64
(
    rm -f {clingo,gringo,reify}-{${MAJOR},${MINOR}} {clingo,gringo,reify}

    for x in clingo gringo reify; do
        ln -s ${prefix}/bin/${x} ${x}-${VERSION}
        ln -s ${x}-${VERSION} ${x}-${MINOR}
        ln -s ${x}-${MINOR}  ${x}-${MAJOR}
        ln -s ${x}-${MAJOR}  ${x}
    done
)
