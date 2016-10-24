#!/bin/bash
set -xe

VERSION="$(grep '#define CLINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libgringo/clingo.h | colrm 1 23 | tr -d '"')"
MINOR=${VERSION%.*}
MAJOR=${MINOR%.*}

mkdir -p /home/wv/bin/linux/64/lib/{cclingo-${VERSION},luaclingo-${VERSION},pyclingo-${VERSION}}

scons --build-dir=release -j5
scons --build-dir=release -j5 libclingo
strip build/release/clingo; cp build/release/clingo /home/wv/bin/linux/64/clingo-${VERSION}
strip build/release/gringo; cp build/release/gringo /home/wv/bin/linux/64/gringo-${VERSION}
strip build/release/reify; cp build/release/reify /home/wv/bin/linux/64/reify-${VERSION}
strip build/release/python/clingo.so; cp build/release/python/clingo.so /home/wv/bin/linux/64/lib/pyclingo-${VERSION}/clingo.so
strip build/release/lua/clingo.so; cp build/release/lua/clingo.so /home/wv/bin/linux/64/lib/luaclingo-${VERSION}/clingo.so
cp libgringo/clingo.h libgringo/clingo.hh /home/wv/bin/linux/64/lib/cclingo-${VERSION}/
strip build/release/libclingo.so; cp build/release/libclingo.so /home/wv/bin/linux/64/lib/cclingo-${VERSION}/libclingo.so

cd /home/wv/bin/linux/64
rm -f {clingo,gringo,reify}-{${MAJOR},${MINOR}} {clingo,gringo,reify}

for x in clingo gringo reify; do
    ln -s ${x}-${VERSION} ${x}-${MINOR}
    ln -s ${x}-${MINOR}   ${x}-${MAJOR}
    ln -s ${x}-${MAJOR}   ${x}
done

cd lib
rm -f {pyclingo,luaclingo,cclingo}-{${MAJOR},${MINOR}} {pyclingo,luaclingo,cclingo}
for x in pyclingo luaclingo cclingo; do
    ln -s ${x}-${VERSION} ${x}-${MINOR}
    ln -s ${x}-${MINOR}   ${x}-${MAJOR}
    ln -s ${x}-${MAJOR}   ${x}
done
