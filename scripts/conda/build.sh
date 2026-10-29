#!/bin/bash

set -euo pipefail

cmake -S . -B build -G Ninja \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_C_COMPILER="${CC}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCLINGO_MANAGE_RPATH=OFF \
    -DCLINGO_BUILD_TESTS=ON \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DPYCLINGO_INSTALL_DIR="${SP_DIR}" \
    -DPython_FIND_STRATEGY="LOCATION" \
    -DPython_ROOT_DIR="${PREFIX}" \
    -DPython_EXECUTABLE="${PYTHON}"

cmake --build build
ctest --test-dir build
cmake --build build --target install
