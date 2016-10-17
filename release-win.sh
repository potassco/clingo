#!/bin/bash

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

VERSION="$(grep '#define GRINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libgringo/gringo/version.hh | grep -o '[0-9]\+.[0-9]\+.[0-9]\+')"
GRINGO="clingo-${VERSION}"
GRINGO_WIN64="${GRINGO}-win64"
GRINGO_WIN32="${GRINGO}-win32"
SRC=$PWD

export PATH=$PATH:/c/Program\ Files\ \(x86\)/GnuWin32/bin

cat << EOF > compile.bat
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsMSBuildCmd.bat"
msbuild /t:clingo;gringo,cclingo,reify,lpconvert /p:Configuration=Release /p:Platform=x64 Clingo.sln
msbuild /t:clingo;gringo,cclingo,reify,lpconvert /p:Configuration=Release /p:Platform=x86 Clingo.sln
msbuild /t:clingo;gringo,pyclingo /p:Configuration=ReleaseScript /p:Platform=x86 Clingo.sln
msbuild /t:clingo;gringo,pyclingo /p:Configuration=ReleaseScript /p:Platform=x64 Clingo.sln
EOF
winpty compile.bat

set -ex

rm -rf "release-${VERSION}"
mkdir -p "release-${VERSION}"
cd "release-${VERSION}"

function copy_files() {
    cp "${SRC}/libgringo/clingo.h" "$2/c-api/"
    cp "${SRC}/libgringo/clingo.hh" "$2/c-api/"
    cp "${SRC}/third-party/INSTALL" "$2"
    cp "${SRC}/"{CHANGES,COPYING} "$2"
    cp "${SRC}/README.md" "$2/README"
    cat "${SRC}/README.md" | grep -v '^\[' > "${2}/README"
    cp -r "${SRC}/examples/" "$2/"
    if [[ ${1} = "zip" ]]; then
        zip -r "${2}.zip" "${2}"
    else
        tar czf "${2}.tar.gz" "${2}"
    fi
}

mkdir -p "${GRINGO_WIN32}/"{c-api,python-api}
cp "${SRC}/ReleaseScript/clingo.exe" "${GRINGO_WIN32}/clingo-script.exe"
cp "${SRC}/ReleaseScript/gringo.exe" "${GRINGO_WIN32}/gringo-script.exe"
cp "${SRC}/ReleaseScript/clingo.pyd" "${GRINGO_WIN32}/python-api/"
cp "${SRC}/Release/clingo.exe" "${GRINGO_WIN32}/"
cp "${SRC}/Release/gringo.exe" "${GRINGO_WIN32}/"
cp "${SRC}/Release/lpconvert.exe" "${GRINGO_WIN32}/"
cp "${SRC}/Release/reify.exe" "${GRINGO_WIN32}/"
cp "${SRC}/Release/clingo.dll" "${GRINGO_WIN32}/c-api/"
cp "${SRC}/Release/clingo.lib" "${GRINGO_WIN32}/c-api/"
cp -r "${SRC}/third-party/win32/third-party" "${GRINGO_WIN32}/"
copy_files zip "${GRINGO_WIN32}"

mkdir -p ${GRINGO_WIN64}/{c-api,python-api}
cp "${SRC}/x64/ReleaseScript/clingo.exe" "${GRINGO_WIN64}/clingo-script.exe"
cp "${SRC}/x64/ReleaseScript/gringo.exe" "${GRINGO_WIN64}/gringo-script.exe"
cp "${SRC}/x64/ReleaseScript/clingo.pyd" "${GRINGO_WIN64}/python-api/"
cp "${SRC}/x64/Release/clingo.exe" "${GRINGO_WIN64}/"
cp "${SRC}/x64/Release/gringo.exe" "${GRINGO_WIN64}/"
cp "${SRC}/x64/Release/lpconvert.exe" "${GRINGO_WIN64}/"
cp "${SRC}/x64/Release/reify.exe" "${GRINGO_WIN64}/"
cp "${SRC}/x64/Release/clingo.dll" "${GRINGO_WIN64}/c-api/"
cp "${SRC}/x64/Release/clingo.lib" "${GRINGO_WIN64}/c-api/"
cp -r "${SRC}/third-party/win64/third-party" "${GRINGO_WIN64}/"
copy_files zip "${GRINGO_WIN64}"
