#!/bin/bash

dev_branch=wip

function list() {
    curl \
      -X GET \
      -H "Accept: application/vnd.github.v3+json" \
      "https://api.github.com/repos/potassco/clingo/actions/workflows" \
      -d "{\"ref\":\"ref\"}"
}

function dispatch() {
    token=$(grep -A1 workflow_dispatch ~/.tokens | tail -n 1)
    curl \
      -u "rkaminsk:$token" \
      -X POST \
      -H "Accept: application/vnd.github.v3+json" \
      "https://api.github.com/repos/potassco/clingo/actions/workflows/$1/dispatches" \
      -d "{\"ref\":\"${dev_branch}\",\"wip\":$2}"
}

wip=true

case $1 in
    list)
        list
        ;;
    release)
        wip=false
        ;&
    dev)
        # .github/workflows/manylinux.yml
        dispatch 4811844 $wip
        # .github/workflows/pipsource.yml
        dispatch 4823035 $wip
        # .github/workflows/ppa-dev.yml
        dispatch 4881510 $wip
        # .github/workflows/conda-dev.yml
        dispatch 4923491 $wip
        # .github/workflows/pipwinmac-wip.yml
        dispatch 4978730 $wip
        ;;
    *)
        echo "usage: trigger {list,dev,release}"
        ;;
esac
