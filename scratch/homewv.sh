#!/bin/bash

set -ex

root="$(readlink -f "$(dirname $0)/..")"
bin=/home/wv/bin/linux/64

cd "${root}"
clingo="$(pwd -P)"
revision="$(git rev-parse --short HEAD)"

mkdir -p build/banane
cd build/banane
cd "$(pwd -P)"

prefix=/home/wv/opt/clingo-banane

cmake "${clingo}" \
    -DCLINGO_BUILD_REVISION="${revision}" \
    -DCMAKE_BUILD_TYPE=release \
    -DCMAKE_INSTALL_PREFIX="${prefix}" \
    -DPYCLINGO_INSTALL_DIR="${prefix}/lib/python/2.7" \
    -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.1" \
    -DCLINGO_CLINGOPATH='"/home/wv/opt/clingo-banane/lib/clingo"' \
    -DLUA_INCLUDE_DIR="/usr/include/lua5.1" \
    -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.1.so" \
    -DCMAKE_VERBOSE_MAKEFILE=True

make -j7 verbose=1
make install

cd "${prefix}"
find . -type f -a -not -type l -a \( -executable -o -name "*.so" -o -name "*.so.*" \) -print0 | xargs -0 strip

cd "${bin}"
for x in gringo clingo reify; do
    ln -fs "${prefix}/bin/${x}" "${x}-banane"
done

# debug

cd "${root}"
mkdir -p build/banane-dbg
cd build/banane-dbg
cd "$(pwd -P)"

prefix=/home/wv/opt/clingo-banane-dbg

cmake "${clingo}" \
    -DCLINGO_BUILD_REVISION="${revision}" \
    -DCMAKE_CXX_FLAGS="-DPy_TRACE_REFS" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPYTHON_EXECUTABLE=/usr/bin/python-dbg \
    -DPYTHON_INCLUDE_DIR:PATH=/usr/include/python2.7_d \
    -DPYTHON_LIBRARY:FILEPATH=/usr/lib/x86_64-linux-gnu/libpython2.7_d.so \
    -DCMAKE_INSTALL_PREFIX="${prefix}" \
    -DPYCLINGO_INSTALL_DIR="${prefix}/lib/python/2.7" \
    -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.1" \
    -DCLINGO_CLINGOPATH='"/home/wv/opt/clingo-banane-dbg/lib/clingo"' \
    -DLUA_INCLUDE_DIR="/usr/include/lua5.1" \
    -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.1.so" \
    -DCMAKE_VERBOSE_MAKEFILE=True


make -j7
make install

cd "${bin}"
for x in gringo clingo reify; do
    ln -fs "${prefix}/bin/${x}" "${x}-banane-dbg"
done

