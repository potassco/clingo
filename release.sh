#!/bin/zsh

# NOTE: simple script to ease releasing binaries
#       meant for internal purposes

VERSION="$(grep '#define CLINGO_VERSION "[^.]\+.[^.]\+.[^.]\+"' libgringo/clingo.h | colrm 1 23 | tr -d '"')"
GRINGO="clingo-${VERSION}"
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

[[ -e ${SRC} ]] || git clone --branch "v${VERSION}" --single-branch git@github.com:potassco/clingo ${SRC}
rm -rf ${SRC}/{.clang_complete,.ycm_extra_conf.py,scratch,gource.sh,python.supp,TODO,sync-clasp.sh,release.sh}

cat "${SRC}/README.md" > README 
echo >> README
sed -e '1,1s/^/Changes in /' -e '1,1s/$/:\n/' -e '2,${/^[^ ]/,$d}' "${SRC}/CHANGES" >> README

function copy_files() {
    rsync "${SRC}/"{CHANGES,COPYING} "$2"
    rsync "${SRC}/README.md" "$2/README"
    rsync -r "${SRC}/examples/" "$2/examples"
    if [[ ${1} = "zip" ]]; then
        zip -r "${2}.zip" "${2}"
    else
        tar czf "${2}.tar.gz" "${2}"
    fi
}

# {{{1 build for linux x86_64

ssh -T "${LIN64}" "mkdir -p '${TEMP}'"
rsync -ar "${SRC}/" "${LIN64}:${TEMP}/${SRC}"

ssh -T "${LIN64}" <<EOF
set -ex

module load scons gcc/4.9.1 bison re2c python/2.7

cd "${TEMP}/${SRC}"
mkdir -p build

cat << EOC > build/release.py
CXX = 'g++'
CXXFLAGS = ['-std=c++11', '-O3', '-Wall']
CPPPATH = ['/cvos/shared/apps/lua/5.2.3/include']
CPPDEFINES = {'NDEBUG': 1}
LIBS = []
LIBPATH = ['/cvos/shared/apps/lua/5.2.3/lib']
LINKFLAGS = ['-std=c++11', '-O3', '-static']
RPATH = []
AR = 'ar'
ARFLAGS = ['rc']
RANLIB = 'ranlib'
BISON = 'bison'
RE2C = 're2c'
WITH_PYTHON = None
WITH_LUA = ['lua', 'dl']
WITH_THREADS = 'posix'
EOC

scons --build-dir=release -j8 gringo clingo reify lpconvert

g++ -o build/release/clingo -std=c++11 -O3 -static build/release/app/clingo/src/main.o build/release/app/clingo/src/clingo_app.o build/release/app/clingo/src/clasp/clasp_app.o -Lbuild/release -L/cvos/shared/apps/lua/5.2.3/lib build/release/libclingo.a build/release/libgringo.a build/release/libreify.a build/release/libclasp.a build/release/libprogram_opts.a build/release/liblp.a -llua -ldl -lpthread -Wl,-u,pthread_cond_broadcast,-u,pthread_cond_destroy,-u,pthread_cond_signal,-u,pthread_cond_timedwait,-u,pthread_cond_wait,-u,pthread_create,-u,pthread_detach,-u,pthread_equal,-u,pthread_getspecific,-u,pthread_join,-u,pthread_key_create,-u,pthread_key_delete,-u,pthread_mutex_lock,-u,pthread_mutex_unlock,-u,pthread_once,-u,pthread_setspecific

strip build/release/gringo
strip build/release/clingo
strip build/release/reify
strip build/release/lpconvert
EOF

mkdir -p "${GRINGO_LIN64}"
for x in build/release/{gringo,clingo,reify,lpconvert}; do
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
CPPPATH = ['/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7', '/opt/local/include']
CPPDEFINES = {'NDEBUG': 1}
LIBS = []
LIBPATH = ['/System/Library/Frameworks/Python.framework/Versions/2.7/lib', '/opt/local/lib']
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

/opt/local/bin/scons --build-dir=release -j8 gringo clingo reify pyclingo lpconvert

strip build/release/gringo
strip build/release/clingo
strip build/release/reify
strip build/release/lpconvert
strip build/release/python/clingo.so 2> /dev/null || true
EOF

mkdir -p "${GRINGO_MAC}/python"
for x in build/release/{gringo,clingo,reify,lpconvert}; do
    rsync "${MAC}:${TEMP}/${SRC}/${x}" "${GRINGO_MAC}"
done

rsync "${MAC}:${TEMP}/${SRC}/build/release/python/clingo.so" "${GRINGO_MAC}/python"
copy_files tgz "${GRINGO_MAC}"

