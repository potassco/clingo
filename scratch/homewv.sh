#!/bin/bash
set -xe

cd "$(dirname $0)/.."

VERSION="banane"
unset PYTHONPATH
source="$(pwd -P)"
revision="$(git rev-parse --short HEAD)"

for VERSION in banane banane-dbg; do

case $VERSION in
    *-dbg)
        suffix="-dbg"
        flags="-DCMAKE_CXX_FLAGS=-DPy_TRACE_REFS"
        build_type=Debug
        ;;
    *)
        suffix=""
        flags=""
        build_type=Release
esac

prefix=/home/wv/opt/clingo-${VERSION}
bin=/home/wv/bin/linux/64

mkdir -p ${prefix}

mkdir -p build/${VERSION}
(
    cd build/${VERSION}
    cd "$(pwd -P)"
    cmake "${source}" ${flags} \
        -DCLINGO_CLINGOPATH="\"${prefix}/lib/clingo\"" \
        -DCLINGO_BUILD_REVISION="${revision}" \
        -DCMAKE_BUILD_TYPE=${build_type} \
        -DCMAKE_VERBOSE_MAKEFILE=On \
        -DCLINGO_BUILD_TESTS=On \
        -DCLASP_BUILD_TESTS=On \
        -DLIB_POTASSCO_BUILD_TESTS=On \
        -DCLINGO_BUILD_EXAMPLES=On \
        -DPYTHON_EXECUTABLE="/usr/bin/python2.7${suffix}" \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DPYCLINGO_INSTALL_DIR="${prefix}/lib/python/2.7" \
        -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.3" \
        -DLUA_INCLUDE_DIR="/usr/include/lua5.3" \
        -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.3.so" \
        -DCMAKE_EXE_LINKER_FLAGS="-s -static-libgcc -static-libstdc++" \
        -DCMAKE_MODULE_LINKER_FLAGS="-s -static-libgcc -static-libstdc++" \
        -DCMAKE_SHARED_LINKER_FLAGS="-s -static-libgcc -static-libstdc++"
    make -j8
    make test
    make install
)

mkdir -p build/py35-${VERSION}
(
    cd build/py35-${VERSION}
    cd "$(pwd -P)"
    cmake "${source}" ${flags} \
        -DCLINGO_CLINGOPATH="\"${prefix}/lib/clingo\"" \
        -DCLINGO_BUILD_REVISION="${revision}" \
        -DCMAKE_BUILD_TYPE=${build_type} \
        -DCMAKE_VERBOSE_MAKEFILE=On \
        -DCLINGO_USE_LIB=On \
        -DCLINGO_BUILD_WITH_LUA=Off \
        -DPYTHON_EXECUTABLE="/usr/bin/python3.5${suffix}" \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DPYCLINGO_INSTALL_DIR="${prefix}/lib/python/3.5" \
        -DCMAKE_MODULE_LINKER_FLAGS="-s -static-libgcc -static-libstdc++ -L${prefix}/lib" \
        -DCMAKE_SHARED_LINKER_FLAGS="-s -static-libgcc -static-libstdc++ -L${prefix}/lib"
    make -j8
    make install
)


mkdir -p build/lua51-${VERSION}
(
    cd build/lua51-${VERSION}
    cd "$(pwd -P)"
    cmake "${source}" ${flags} \
        -DCLINGO_CLINGOPATH="\"${prefix}/lib/clingo\"" \
        -DCLINGO_BUILD_REVISION="${revision}" \
        -DCMAKE_BUILD_TYPE=${build_type} \
        -DCMAKE_VERBOSE_MAKEFILE=On \
        -DCLINGO_USE_LIB=On \
        -DCLINGO_BUILD_WITH_PYTHON=Off \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.1" \
        -DLUA_INCLUDE_DIR="/usr/include/lua5.1" \
        -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.1.so" \
        -DCMAKE_MODULE_LINKER_FLAGS="-s -static-libgcc -static-libstdc++ -L${prefix}/lib" \
        -DCMAKE_SHARED_LINKER_FLAGS="-s -static-libgcc -static-libstdc++ -L${prefix}/lib"
    make -j8
    make install
)

mkdir -p build/lua52-${VERSION}
(
    cd build/lua51-${VERSION}
    cd "$(pwd -P)"
    cmake "${source}" ${flags} \
        -DCLINGO_CLINGOPATH="\"${prefix}/lib/clingo\"" \
        -DCLINGO_BUILD_REVISION="${revision}" \
        -DCMAKE_BUILD_TYPE=${build_type} \
        -DCMAKE_VERBOSE_MAKEFILE=On \
        -DCLINGO_USE_LIB=On \
        -DCLINGO_BUILD_WITH_PYTHON=Off \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DLUACLINGO_INSTALL_DIR="${prefix}/lib/lua/5.2" \
        -DLUA_INCLUDE_DIR="/usr/include/lua5.2" \
        -DLUA_LIBRARY="/usr/lib/x86_64-linux-gnu/liblua5.2.so" \
        -DCMAKE_MODULE_LINKER_FLAGS="-s -static-libgcc -static-libstdc++ -L${prefix}/lib" \
        -DCMAKE_SHARED_LINKER_FLAGS="-s -static-libgcc -static-libstdc++ -L${prefix}/lib"
    make -j8
    make install
)

(
cd "${bin}"
for x in gringo clingo reify; do
    ln -fs "${prefix}/bin/${x}" "${x}-banane${suffix}"
done
)

done
