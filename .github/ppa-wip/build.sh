#!/bin/bash

function usage {
    echo "./$(basename $0) {bionic,focal} {create,sync,build,put,clean}*"
}

if [[ $# < 1 ]]; then
    usage
    exit 1
fi

i=1
rep="${!i}"
shift

case "$rep" in
    focal|bionic)
        ;;
    *)
        usage
        exit 1
esac

for act in "${@}"; do
    echo $act
    case "$act" in
        create)
            sudo pbuilder create --basetgz /var/cache/pbuilder/${rep}.tgz --distribution ${rep} --debootstrapopts --variant=buildd
            ;;
        sync)
            rsync -aq \
                --exclude __pycache__ \
                --exclude .mypy_cache \
                --exclude '*.egg-info' \
                --exclude dist \
                --exclude build \
                ../../app \
                ../../clasp \
                ../../libgringo \
                ../../libclingo \
                ../../libluaclingo \
                ../../libpyclingo \
                ../../libpyclingo_cffi \
                ../../libreify \
                ../../cmake \
                ../../CMakeLists.txt \
                ../../CHANGES.md \
                ../../README.md \
                ../../INSTALL.md \
                ../../LICENSE.md \
                $rep/
            ;;
        changes)
            VERSION=$(sed -n '/#define CLINGO_VERSION "/s/.*"\([0-9]\+\.[0-9\+]\.[0-9]\+\)".*/\1/p' ../../libclingo/clingo.h)
            BUILD=$(curl -sL http://ppa.launchpad.net/potassco/${rep}-wip/ubuntu/pool/main/c/clingo/ | sed -n '/\.dsc/s/.*alpha\([0-9]\+\).*/\1/p' | sort -rn | head -1)
            BUILD=$[BUILD+1]
            cat > ${rep}/debian/changelog <<EOF
clingo (${VERSION}-alpha$[BUILD+1]) ${rep}; urgency=medium

  * build for git revision $(git rev-parse HEAD)

 -- Roland Kaminski <kaminski@cs.uni-potsdam.de>  $(date -R)
EOF
            ;;
        build)
            VERSION="$(head -n 1 ${rep}/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
            (cd "${rep}" && pdebuild -- --basetgz /var/cache/pbuilder/${rep}.tgz; debsign -k744d959e10f5ad73f9cf17cc1d150536980033d5 ../clingo_${VERSION}_source.changes)
            ;;
        put)
            VERSION="$(head -n 1 ${rep}/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
            dput ppa:potassco/${rep}-wip clingo_${VERSION}_source.changes
            ;;
        clean)
            rm -rf \
                "$rep"/app \
                "$rep"/clasp \
                "$rep"/libgringo \
                "$rep"/libclingo \
                "$rep"/libluaclingo \
                "$rep"/libpyclingo \
                "$rep"/libpyclingo_cffi \
                "$rep"/libreify \
                "$rep"/cmake \
                "$rep"/CMakeLists.txt \
                "$rep"/CHANGES.md \
                "$rep"/README.md \
                "$rep"/INSTALL.md \
                "$rep"/LICENSE.md \
                "$rep"/debian/files \
                "$rep"/debian/.debhelper \
                "$rep"/debian/clingo.debhelper.log \
                "$rep"/debian/clingo.substvars \
                "$rep"/debian/clingo \
                "$rep"/debian/debhelper-build-stamp \
                "$rep"/debian/libclingo-dev.debhelper.log \
                "$rep"/debian/libclingo-dev.substvars \
                "$rep"/debian/libclingo-dev/ \
                "$rep"/debian/libclingo.debhelper.log \
                "$rep"/debian/libclingo.substvars \
                "$rep"/debian/libclingo \
                "$rep"/debian/python3-clingo.debhelper.log \
                "$rep"/debian/python3-clingo.postinst.debhelper \
                "$rep"/debian/python3-clingo.prerm.debhelper \
                "$rep"/debian/python3-clingo.substvars \
                "$rep"/debian/python3-clingo \
                "$rep"/debian/tmp \
                "$rep"/obj-x86_64-linux-gnu \
                *.build \
                *.deb \
                *.dsc \
                *.buildinfo \
                *.changes \
                *.ddeb \
                *.tar.xz \
                *.upload
            ;;
        *)
            usage
            exit 1
    esac
done
