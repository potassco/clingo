#!/bin/bash

set -ex

mkdir -p build
cd build

if [ -z "${PYTHON}" ]; then
    PYTHON="$(which python)"
fi

cmake .. \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_C_COMPILER="${CC}" \
    -DPython_ROOT_DIR="${PREFIX}" \
    -DPython_EXECUTABLE="${PYTHON}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCLINGO_MANAGE_RPATH=OFF \
    -DPYCLINGO_INSTALL_DIR="${SP_DIR}" \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_BUILD_TYPE=Release

make -j"${CPU_COUNT}"
make install
