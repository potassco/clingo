#!/bin/bash

set -ex

bin=/home/wv/bin/linux/64
clingo="$(pwd -P)"
prefix=/home/wv/opt/clingo-banane

cd "$(dirname $0)/.."
mkdir -p build/banane
cd build/banane
cd "$(pwd -P)"

cmake "${clingo}" \
    -DCMAKE_BUILD_TYPE=release \
    -DCMAKE_INSTALL_PREFIX="${prefix}" \
    -DPYCLINGO_INSTALL_DIR="${prefix}/lib/python/2.7" \
    -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.1" \
    -DCMAKE_CXX_COMPILER="/home/wv/bin/linux/64/gcc-4.9/bin/g++-4.9" \
    -DCMAKE_C_COMPILER="/home/wv/bin/linux/64/gcc-4.9/bin/gcc-4.9" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath=/home/wv/bin/linux/64/gcc-4.9/lib" \
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-rpath=/home/wv/bin/linux/64/gcc-4.9/lib" \
    -DCLINGO_CLINGOPATH='"/home/wv/opt/clingo-banane/lib/clingo"' \
    -DLUA_INCLUDE_DIR="/usr/include/lua5.1" \
    -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.1.so"

make -j7
make install

cd "${prefix}"
find . -type f -a -not -type l -a \( -executable -o -name "*.so" -o -name "*.so.*" \) -print0 | xargs -0 strip

cd "${bin}"
for x in gringo clingo reify; do
    ln -fs "${prefix}/bin/${x}" "${x}-banane"
done
