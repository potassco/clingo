#!/bin/bash

function usage {
    echo "./$(basename "$0") {stable,wip} {resolute,noble,jammy,focal,bionic} {create,sync,changes,build,put,clean}*"
}

if [[ $# -lt 1 ]]; then
    usage
    exit 1
fi

ref="${1}"
shift

case "${ref}" in
stable | wip)
    ;;
*)
    usage
    exit 1
    ;;
esac

rep="${1}"
shift

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
PROJECT_NAME="$("${PROJECT_ROOT}/scratch/get_config.py" ppa name)"
ADD_PPA="$("${PROJECT_ROOT}/scratch/get_config.py" ppa add_ppa no)"

case "${rep}" in
resolute | noble | jammy | focal | bionic)
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
        apt-get install -y software-properties-common
        if [[ "${ADD_PPA}" == "yes" ]]; then
            add-apt-repository -y "ppa:potassco/${ref}"
        fi
        apt-get update
        apt-get install -y tree debhelper
        ;;
    create)
        sudo pbuilder create --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz" --distribution "${rep}" --debootstrapopts --variant=buildd
        sudo pbuilder execute --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz" --save-after-exec -- build.sh "${ref}" "${rep}" _ppa
        ;;
    sync)
        SYNC=$(./scratch/get_config.py ppa sync)
        mapfile -t SYNC < <(script)
        rsync -aq "${SYNC[@]}" "$rep/"
        PROJECT_NAME_UPPER="${PROJECT_NAME^^})"
        sed -i "s/export ${PROJECT_NAME_UPPER//-/}_BUILD_REVISION =.*/export ${PROJECT_NAME_UPPER//-/}_BUILD_REVISION = $(git rev-parse --short HEAD)/" "${rep}/debian/rules"
        ;;
    changes)
        VERSION="$(python ../../scratch/version.py)"
        BUILD=$(curl -sL "http://ppa.launchpad.net/potassco/${ref}/ubuntu/pool/main/c/${PROJECT_NAME}/" | sed -n "/${VERSION//./\\.}-${rep}[0-9]\+\.dsc/s/.*${rep}\([0-9]\+\).*/\1/p" | sort -rn | head -1)
        cat >"${rep}/debian/changelog" <<EOF
${PROJECT_NAME} (${VERSION}-${rep}$((BUILD + 1))) ${rep}; urgency=medium

  * build for git revision $(git rev-parse HEAD)

 -- Roland Kaminski <kaminski@cs.uni-potsdam.de>  $(date -R)
EOF
        ;;
    build)
        VERSION="$(head -n 1 ${rep}/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
        (
            cd "${rep}"
            pdebuild --buildresult . -- --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz"
        )
        KEY="$(./scratch/get_config.py ppa key)"
        debsign --re-sign -k"${KEY}" "${PROJECT_NAME}_${VERSION}_source.changes"
        ;;
    put)
        VERSION="$(head -n 1 "${rep}/debian/changelog" | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
        dput "ppa:potassco/${ref}" "${PROJECT_NAME}_${VERSION}_source.changes"
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
