#!/bin/bash

baseurl="https://svn.cs.uni-potsdam.de/svn/reposWV/Privat/BenjaminKaufmann/trunk/clasp3.v2"

# sync libprogram_opts
(
    cd libprogram_opts
    svn export --force "$baseurl/libprogram_opts/src" src
    svn export --force "$baseurl/libprogram_opts/program_opts" program_opts
)

# sync clasp app and lpconvert app
(
    cd app
    svn export --non-recursive --force "$baseurl/app/" clingo/src/clasp
    rm -f clingo/src/clasp/main.cpp
    rm -f clingo/src/clasp/CMakeLists.txt
    svn export --non-recursive --force "$baseurl/app/lpconvert" lpconvert
)

# sync libclasp
(
    cd libclasp
    svn export --force "$baseurl/libclasp/src" src
    svn export --force "$baseurl/libclasp/clasp" clasp
)
# sync liblp
(
    cd liblp
    svn export --force "$baseurl/liblp/src" src
    svn export --force "$baseurl/liblp/potassco" potassco
    svn export --force "$baseurl/liblp/tests" tests
)

version=$(svn info "$baseurl" | grep "Last Changed Rev:" | colrm 1 18)
#sed -i "s/\\\$Revision\\\$/$version/" libclasp/clasp/clasp_facade.h
sed -i "s/#define CLASP_VERSION \"\(.*\)\"/#define CLASP_VERSION \"\1-R$version\"/g" libclasp/clasp/clasp_facade.h

