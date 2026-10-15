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

set -e

function project_root {
    (cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)
}

function get_config {
    python3 "$(project_root)/scratch/get_config.py" ppa "${@}"
}

function project_name {
    get_config name
}

function project_version {
    python3 "$(project_root)/scratch/version.py"
}

case "${rep}" in
resolute | noble | jammy | focal | bionic)
    ;;
*)
    usage
    exit 1
    ;;
esac

if [[ $# -gt 1 && "$1" == "_ppa" ]]; then
    if [[ $# -ne 2 ]]; then
        usage
        exit 1
    fi
    apt-get install -y software-properties-common
    if [[ "$2" == "yes" ]]; then
        add-apt-repository -y "ppa:potassco/${ref}"
    fi
    apt-get update
    apt-get install -y tree python3 debhelper
    exit 0
fi

for act in "${@}"; do
    echo "${act}"
    case "${act}" in
    create)
        sudo pbuilder create --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz" --distribution "${rep}" --debootstrapopts --variant=buildd
        sudo pbuilder execute --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz" --save-after-exec -- build.sh "${ref}" "${rep}" _ppa "$(get_config add_ppa no)"
        ;;
    sync)
        mapfile -t include < <(get_config include)
        mapfile -t exclude < <(get_config exclude)
        args=()
        for x in "${exclude[@]}"; do
            args+=(--exclude "${x}")
        done
        for x in "${include[@]}"; do
            mapfile -t y < <(compgen -G "${x}")
            args+=("${y[@]}")
        done
        rsync -aq "${args[@]}" "$rep/"

        name="$(project_name)"
        name="${name^^}"
        name="${name//-/}"
        sed -i "s/export ${name}_BUILD_REVISION =.*/export ${name}_BUILD_REVISION = $(git rev-parse --short HEAD)/" "${rep}/debian/rules"
        ;;
    changes)
        version="$(project_version)"
        name="$(project_name)"
        build=$(curl -sL "http://ppa.launchpad.net/potassco/${ref}/ubuntu/pool/main/c/${name}/" | sed -n "/${version//./\\.}-${rep}[0-9]\+\.dsc/s/.*${rep}\([0-9]\+\).*/\1/p" | sort -rn | head -1)
        cat >"${rep}/debian/changelog" <<EOF
${name} (${version}-${rep}$((build + 1))) ${rep}; urgency=medium

  * build for git revision $(git rev-parse HEAD)

 -- Roland Kaminski <kaminski@cs.uni-potsdam.de>  $(date -R)
EOF
        ;;
    build)
        version="$(head -n 1 ${rep}/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
        (
            cd "${rep}"
            pdebuild --buildresult . -- --basetgz "/var/cache/pbuilder/${ref}-${rep}.tgz"
        )
        key="$(get_config key)"
        name="$(project_name)"
        debsign --re-sign -k"${key}" "${name}_${version}_source.changes"
        ;;
    put)
        version="$(head -n 1 "${rep}/debian/changelog" | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
        name="$(project_name)"
        dput "ppa:potassco/${ref}" "${name}_${version}_source.changes"
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
