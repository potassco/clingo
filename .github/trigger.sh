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
      -d "{\"ref\":\"${dev_branch}\"}"
}

case $1 in
    list)
        list
        ;;
    dev)
        # .github/workflows/manylinux.yml
        dispatch 4811844
        # .github/workflows/pipsource.yml
        dispatch 4823035
        # .github/workflows/ppa-dev.yml
        dispatch 4881510
        # .github/workflows/conda-dev.yml
        dispatch 4923491
        # .github/workflows/pipwinmac-wip.yml
        dispatch 4978730
        ;;
    release)
        echo "implement me"
        ;;
    *)
        echo "usage: trigger {list,dev,release}"
        ;;
esac
