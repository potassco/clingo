#!/bin/bash

type=$1
repo=$2

if [[ "${type}" == ppa ]]; then
    apt-get install -y software-properties-common gpg-agent
    if [[ -n $repo ]]; then
        add-apt-repository -y "ppa:${repo}"
        apt-get update
    fi
else
    if [[ -n $repo ]]; then
        apt-get install -y curl
        curl -1sLf "https://dl.cloudsmith.io/public/${repo}/setup.deb.sh" | bash
        apt-get update
    fi
fi
apt-get install -y tree debhelper
