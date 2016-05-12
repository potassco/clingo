#!/bin/bash
svn update
make release -j5
make mingw32 target=all -j5
rm -rf gringo-win32 gringo-win32.zip
mkdir gringo-win32
cp build/mingw32/bin/{gringo,clingo,iclingo}.exe gringo-win32
mingw32-strip gringo-win32/*.exe
zip -r gringo-win32.zip gringo-win32
scp gringo-win32.zip belle-ile:~/public_html/gringo
rm -rf gringo-win32 gringo-win32.zip

