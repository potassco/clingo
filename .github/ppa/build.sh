#!/bin/bash

function usage {
    echo "./$(basename "$0") --type={ppa,cloudsmith} --build-number <n> {wip-20} {noble,resolute,trixie} {create,sync,changes,build,put,clean}*"
}

if [[ $# -lt 1 ]]; then
    usage
    exit 1
fi

BUILD_NUMBER=""
BUILD_TYPE="ppa"

if ! ARGS=$(getopt -o '' --long build-number:,type: -- "$@"); then
    usage
    exit 1
fi

eval set -- "$ARGS"
while true; do
    case "$1" in
    --build-number)
        BUILD_NUMBER="$2"
        shift 2
        ;;
    --type)
        BUILD_TYPE="$2"
        shift 2
        ;;
    --)
        shift
        break
        ;;
    *)
        usage
        exit 1
        ;;
    esac
done

set -ex

ref="${1}"
shift

case "${BUILD_TYPE}" in
ppa) ;;
cloudsmith) ;;
*)
    usage
    exit 1
    ;;
esac

case "${ref}" in
wip-20) ;;
*)
    usage
    exit 1
    ;;
esac

rep="${1}"
shift

DISTRIBUTION=ubuntu
PBUILDER_ARGS=()
case "${rep}" in
resolute) ;;
noble) ;;
trixie)
    PBUILDER_ARGS+=(--mirror http://deb.debian.org/debian)
    DISTRIBUTION=debian
    ;;
*)
    usage
    exit 1
    ;;
esac

for act in "${@}"; do
    echo "${act}"
    case "${act}" in
    _ppa)
        if [[ "${DISTRIBUTION}" == ubuntu ]]; then
            apt-get install -y software-properties-common
            #add-apt-repository -y "ppa:potassco/${ref}"
        else
            # could for example setup cloudforge repository here
            :
        fi
        apt-get update
        apt-get install -y tree debhelper
        ;;
    create)
        sudo apt-get update
        sudo apt-get install pbuilder pbuilder-scripts debootstrap devscripts dh-make dput dh-python
        if [[ "${DISTRIBUTION}" == debian ]]; then
            sudo apt-get install debian-archive-keyring
        fi
        sudo pbuilder create --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz" --distribution "${rep}" "${PBUILDER_ARGS[@]}" --debootstrapopts --variant=buildd
        sudo pbuilder execute --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz" --save-after-exec -- build.sh "${ref}" "${rep}" _ppa
        ;;
    sync)
        rsync -aq \
            --exclude __pycache__ \
            --exclude .mypy_cache \
            --exclude '*,cover' \
            --exclude '*.egg-info' \
            --exclude dist \
            --exclude build \
            ../../app \
            ../../cmake \
            ../../CMakeLists.txt \
            ../../DEVELOP.md \
            ../../lib \
            ../../LICENSE.md \
            ../../README.md \
            ../../third_party \
            "$rep/"
        sed -i "s/export CLINGO_GIT_HASH =.*/export CLINGO_GIT_HASH = $(git rev-parse --short HEAD)/" "${rep}/debian/rules"
        ;;
    changes)
        VERSION="$(sed -n '/#define CLINGO_VERSION "/s/.*"\([0-9]\+\.[0-9\+]\.[0-9]\+\)".*/\1/p' ../../lib/c-api/include/clingo/core.h)"
        if [[ -z "${BUILD_NUMBER}" ]]; then
            echo "No build number given"
            exit 1
        fi
        if [[ "$DISTRIBUTION" == "ubuntu" ]]; then
            SUFFIX="-${rep}${BUILD_NUMBER}"
        else
            SUFFIX=".${BUILD_NUMBER}"
        fi
        cat >"${rep}/debian/changelog" <<EOF
clingo (${VERSION}${SUFFIX}) ${rep}; urgency=medium

  * build for git revision $(git rev-parse HEAD)

 -- Roland Kaminski <kaminski@cs.uni-potsdam.de>  $(date -R)
EOF
        ;;
    build)
        if [[ "${BUILD_TYPE}" == "ppa" ]]; then
            VERSION="$(head -n 1 "${rep}/debian/changelog" | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
            (
                cd "${rep}"
                pdebuild --buildresult . -- --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz"
            )
            debsign -S -k744d959e10f5ad73f9cf17cc1d150536980033d5 "clingo_${VERSION}_source.changes"
        else
            (
                cd "${rep}"
                pdebuild --buildresult . -- --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz"
                rm -f ./*dbgsym*.deb
            )
        fi
        ;;
    put)
        VERSION="$(head -n 1 "${rep}/debian/changelog" | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
        dput "ppa:potassco/${ref}" "clingo_${VERSION}_source.changes"
        ;;
    clean)
        rm -f ./*.{dsc,xz,build,changes}
        git clean -ffdx -- "${rep}"
        git restore -- "${rep}/debian/changelog" "${rep}/debian/rules"
        ;;
    *)
        usage
        exit 1
        ;;
    esac
done
