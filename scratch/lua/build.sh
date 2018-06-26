#!/bin/bash
mkdir -p build
cd build
source "$(which emsdk_env.sh)"
emcmake cmake -DCMAKE_INSTALL_PREFIX=${HOME}/local/opt/lua-js ..
make
make install
