#!/bin/bash
clingo-4 --ifs="\n" "$@" convert.lp | grep "^_" | sed -e 's/$/./' -e 's/^_//'
