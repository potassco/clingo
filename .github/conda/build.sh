#!/bin/bash

set -ex

if [ -z "${PYTHON}" ]; then
    PYTHON="$(which python)"
fi

cmake -S . -B build -G Ninja \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_C_COMPILER="${CC}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPython_ROOT_DIR="${PREFIX}" \
    -DPython_EXECUTABLE="${PYTHON}" \
    -DCLINGO_MANAGE_RPATH=OFF \
    -DCLINGO_BUILD_TESTS=ON \
    -DPYCLINGO_INSTALL_DIR="${SP_DIR}"

cmake --build build
ctest --test-dir build
cmake --build build --target install
