#!/bin/bash

mkdir build
cd build

if [ -z "${PYTHON}" ]; then
    PYTHON="$(which python)"
fi

cmake .. \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_C_COMPILER="${CC}" \
    -DPYTHON_EXECUTABLE="${PYTHON}" \
    -DCLINGO_REQUIRE_PYTHON=ON \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DPYCLINGO_USER_INSTALL=OFF \
    -DCLINGO_BUILD_WITH_LUA=OFF \
    -DCLINGO_MANAGE_RPATH=OFF \
    -DPYCLINGO_INSTALL_DIR="${SP_DIR}" \
    -DCMAKE_BUILD_TYPE=Release

make -j${CPU_COUNT}
make install

