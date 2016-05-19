#!/bin/bash

VERSION=$(grep '^SET(CPACK_PACKAGE_VERSION ".\..\..")$' CMakeLists.txt | sed "s/[^0-9.]//g")

make all_static32 all_mingw32

cd doc/guide
pdflatex guide
bibtex   guide
pdflatex guide
pdflatex guide
cd ../..

rm -rf   build/release
mkdir -p build/release
cd       build/release

# create the windows binary release
WIN32_PATH=binaries-${VERSION}-win32
mkdir -p                              ${WIN32_PATH}
cp ../gringo/mingw32/bin/gringo.exe   ${WIN32_PATH}/
cp ../clingo/mingw32/bin/clingo.exe   ${WIN32_PATH}/
cp ../iclingo/mingw32/bin/iclingo.exe ${WIN32_PATH}/
cp ../../COPYING                      ${WIN32_PATH}/
cp ../../CHANGES                      ${WIN32_PATH}/
cp ../../doc/guide/guide.pdf          ${WIN32_PATH}/
zip -r ${WIN32_PATH}.zip              ${WIN32_PATH}/
rm -rf                                ${WIN32_PATH}/

# create the linux binary release
LINUX_PATH=binaries-${VERSION}-x86-linux
mkdir -p                           ${LINUX_PATH}
cp ../gringo/static32/bin/gringo   ${LINUX_PATH}/
cp ../clingo/static32/bin/clingo   ${LINUX_PATH}/
cp ../iclingo/static32/bin/iclingo ${LINUX_PATH}/
cp ../../COPYING                   ${LINUX_PATH}/
cp ../../CHANGES                   ${LINUX_PATH}/
cp ../../doc/guide/guide.pdf       ${LINUX_PATH}/
tar -czf ${LINUX_PATH}.tar.gz      ${LINUX_PATH}/
rm -rf                             ${LINUX_PATH}/

# create the source release
cd ../gringo/static32
make package_source
cd ../../release

tar -xf ../gringo/static32/gringo-${VERSION}-source.tar.gz
# copy the generated files into the source release
cp ../gringo/static32/libgringo/lparselexer.cpp          gringo-${VERSION}-source/libgringo/src
cp ../gringo/static32/libgringo/plainlparselexer.cpp     gringo-${VERSION}-source/libgringo/src
cp ../gringo/static32/libgringo/lparseconverter_impl.cpp gringo-${VERSION}-source/libgringo/src
cp ../gringo/static32/libgringo/lparseconverter_impl.h   gringo-${VERSION}-source/libgringo/src
cp ../gringo/static32/libgringo/lparseparser_impl.cpp    gringo-${VERSION}-source/libgringo/src
cp ../gringo/static32/libgringo/lparseparser_impl.h      gringo-${VERSION}-source/libgringo/src
tar -czf gringo-${VERSION}-source.tar.gz                 gringo-${VERSION}-source
rm -rf                                                   gringo-${VERSION}-source

#svn copy https://potassco.svn.sourceforge.net/svnroot/potassco/trunk/gringo https://potassco.svn.sourceforge.net/svnroot/potassco/tags/gringo-${VERSION}
