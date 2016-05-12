#!/bin/zsh

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

VERSION="$(grep '#define GRINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libgringo/gringo/version.hh | colrm 1 23 | tr -d '"')"
GRINGO="gringo-${VERSION}"
GRINGO_MAC="${GRINGO}-macos-10.9"
GRINGO_LIN64="${GRINGO}-linux-x86_64"
GRINGO_WIN64="${GRINGO}-win64"
SRC="${GRINGO}-source"
TEMP="/tmp/${GRINGO}-build"
MAC=kaminski_local@herm
LIN64=zuse
WIN64=localhost
WIN64_PORT=2222

set -ex

if ! vboxmanage list runningvms | grep -q '"Fedora" {153088fd-f48f-4ec0-b04b-a6d7f1b5b885}'; then
    DISPLAY=":0" nohup VirtualBox --startvm Fedora &>/dev/null &
    echo "waiting 60 seconds to power up machine (which is hopefully enough)"
    sleep 60
fi

if [[ $1 == clean ]]; then
    ssh -T -p ${WIN64_PORT} "${WIN64}" "rm -rf '${TEMP}'"
    ssh -T "${MAC}" "rm -rf '${TEMP}'"
    ssh -T "${LIN64}" "rm -rf '${TEMP}'"
fi

mkdir -p "release-${VERSION}"
cd "release-${VERSION}"

[[ -e ${SRC} ]] || svn export "http://svn.code.sf.net/p/potassco/code/tags/${GRINGO}" ${SRC}
rm -rf ${SRC}/{.clang_complete,.ycm_extra_conf.py,scratch,gource.sh,python.supp,TODO,sync-clasp.sh,release.sh}

rsync -r "${SRC}/" "${SRC/gringo/clingo}"
tar czf "${SRC}.tar.gz" "${SRC}"
tar czf "${SRC/gringo/clingo}.tar.gz" "${SRC/gringo/clingo}"

cat "${SRC}/README" > README 
echo >> README
sed -e '1,1s/^/Changes in /' -e '1,1s/$/:\n/' -e '2,${/^[^ ]/,$d}' "${SRC}/CHANGES" >> README

function copy_files() {
    rsync "${SRC}/"{CHANGES,COPYING,README} "$2"
    rsync -r "${SRC}/examples/" "$2/examples"
    rsync -r "${2}/" "${2/gringo/clingo}"
    if [[ ${1} = "zip" ]]; then
        zip -r "${2}.zip" "${2}"
        zip -r "${2/gringo/clingo}.zip" "${2/gringo/clingo}"
    else
        tar czf "${2}.tar.gz" "${2}"
        tar czf "${2/gringo/clingo}.tar.gz" "${2/gringo/clingo}"
    fi
}

# {{{1 build for win64

ssh -T -p ${WIN64_PORT} "${WIN64}" "mkdir -p '${TEMP}'"
rsync --rsh="ssh -p ${WIN64_PORT}" -ar "${SRC}/" "${WIN64}:${TEMP}/${SRC}"

ssh -T -p ${WIN64_PORT} "${WIN64}" <<EOF
set -ex

cd "${TEMP}/${SRC}"
mkdir -p build

cat << EOC > build/mingw64.py
CXX = 'x86_64-w64-mingw32-g++'
CXXFLAGS = ['-std=c++11', '-O3', '-Wall', '-W']
CPPPATH = ['/home/kaminski/local/opt/lua-5.1.5-win64/include', '/home/kaminski/local/opt/tbb-4.3.5-win64/include', '/home/kaminski/local/opt/cppunit-1.13.2-win64/include']
CPPDEFINES = {'NDEBUG': 1}
LIBS = []
LIBPATH = ['/home/kaminski/local/opt/lua-5.1.5-win64/lib', '/home/kaminski/local/opt/tbb-4.3.5-win64/lib', '/home/kaminski/local/opt/cppunit-1.13.2-win64/lib']
LINKFLAGS = ['-std=c++11', '-static']
RPATH = []
AR = 'x86_64-w64-mingw32-ar'
ARFLAGS = ['rc']
RANLIB = 'x86_64-w64-mingw32-ranlib'
BISON = 'bison'
RE2C = 're2c'
PYTHON_CONFIG = None
PKG_CONFIG = 'x86_64-w64-mingw32-pkg-config'
WITH_PYTHON = None
WITH_LUA = 'lua'
WITH_TBB = 'tbb'
WITH_CPPUNIT = 'cppunit'
EOC

scons --build-dir=mingw64 -j4 gringo clingo reify

strip build/mingw64/gringo
strip build/mingw64/clingo
strip build/mingw64/reify
EOF

mkdir -p "${GRINGO_WIN64}"
for x in build/mingw64/{gringo,clingo,reify}; do
    rsync --rsh="ssh -p ${WIN64_PORT}" "${WIN64}:${TEMP}/${SRC}/${x}" "${GRINGO_WIN64}/${x#build/mingw64/}.exe"
done
copy_files zip "${GRINGO_WIN64}"

# {{{1 build for linux x86_64

ssh -T "${LIN64}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${LIN64}:${TEMP}/${SRC}"

ssh -T "${LIN64}" <<EOF
set -ex

module load scons gcc/4.8.1 bison re2c python/2.7

cd "${TEMP}/${SRC}"
mkdir -p build

cat << EOC > build/release.py
CXX = 'g++'
CXXFLAGS = ['-std=c++11', '-O3', '-Wall']
CPPPATH = ['/home/kaminski/local/opt/tbb-4.3.5-static/include', '/cvos/shared/apps/lua/5.2.3/include']
CPPDEFINES = {'NDEBUG': 1}
LIBS = []
LIBPATH = ['/home/kaminski/local/opt/tbb-4.3.5-static/lib', '/cvos/shared/apps/lua/5.2.3/lib']
LINKFLAGS = ['-std=c++11', '-O3', '-static', '-pthread']
RPATH = []
AR = 'ar'
ARFLAGS = ['rc']
RANLIB = 'ranlib'
BISON = 'bison'
RE2C = 're2c'
WITH_PYTHON = None
WITH_LUA = ['lua', 'dl']
WITH_TBB = 'tbb'
WITH_CPPUNIT = None
EOC

scons --build-dir=release -j8 gringo clingo reify

strip build/release/gringo
strip build/release/clingo
strip build/release/reify
EOF

mkdir -p "${GRINGO_LIN64}"
for x in build/release/{gringo,clingo,reify}; do
    rsync "${LIN64}:${TEMP}/${SRC}/${x}" "${GRINGO_LIN64}"
done
copy_files tgz "${GRINGO_LIN64}"

# {{{1 build for macos

ssh -T "${MAC}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${MAC}:${TEMP}/${SRC}"

ssh -T "${MAC}" <<EOF
set -ex
cd "${TEMP}/${SRC}"
mkdir -p build

cat << EOC > build/release.py
CXX = 'clang++'
CXXFLAGS = ['-O3', '-W', '-Wall', '-Wno-gnu', '-pedantic', '-std=c++11', '-stdlib=libc++', '-Ibuild/cppunit/include']
CPPPATH = ['/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7', '/Users/kaminski/local/tbb-4.3.5/include', '/opt/local/include']
CPPDEFINES = {'NDEBUG': 1}
LIBS = []
LIBPATH = ['/System/Library/Frameworks/Python.framework/Versions/2.7/lib', '/Users/kaminski/local/tbb-4.3.5/lib', '/opt/local/lib']
LINKFLAGS = ['-std=c++11', '-stdlib=libc++']
RPATH = []
AR = 'ar'
ARFLAGS = ['rc']
RANLIB = 'ranlib'
BISON = '/opt/local/bin/bison'
RE2C = '/opt/local/bin/re2c'
WITH_PYTHON = 'python2.7'
WITH_LUA = '/opt/local/lib/liblua.a'
WITH_TBB = 'tbb_static'
WITH_CPPUNIT = 'cppunit'
EOC

/opt/local/bin/scons --build-dir=release -j8 gringo clingo reify pyclingo

strip build/release/gringo
strip build/release/clingo
strip build/release/reify
strip build/release/python/gringo.so 2> /dev/null || true
EOF

mkdir -p "${GRINGO_MAC}/python"
for x in build/release/{gringo,clingo,reify}; do
    rsync "${MAC}:${TEMP}/${SRC}/${x}" "${GRINGO_MAC}"
done

rsync "${MAC}:${TEMP}/${SRC}/build/release/python/gringo.so" "${GRINGO_MAC}/python"
copy_files tgz "${GRINGO_MAC}"

