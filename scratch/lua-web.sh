#!/bin/bash

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

VERSION=5.3.4

set -ex

mkdir -p /tmp/lua-work
cd /tmp/lua-work

wget -c https://www.lua.org/ftp/lua-${VERSION}.tar.gz
tar -x --transform='s|^[^/]*|lua|' -f lua-${VERSION}.tar.gz

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

add_library(lua STATIC ${CORE} ${LIB})
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

(
    cd lua
    source $(which emsdk_env.sh)
    emcmake cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX="${HOME}/local/opt/lua-${VERSION}-js" .
    make -j8
    make install
)
