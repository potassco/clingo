#!/bin/bash

#baseurl="https://svn.code.sf.net/p/potassco/code/branches/claspD-2.0"
#baseurl="https://svn.cs.uni-potsdam.de/svn/reposWV/Privat/BenjaminKaufmann/trunk/clasp3.0.x"
baseurl="https://svn.cs.uni-potsdam.de/svn/reposWV/Privat/BenjaminKaufmann/trunk/clasp3.v2"

# sync libprogram_opts
(
	cd libprogram_opts
	svn export --force "$baseurl/libprogram_opts/src" src
	svn export --force "$baseurl/libprogram_opts/program_opts" program_opts
)

# sync app
(
	cd app/clingo/src
	svn export --force "$baseurl/app/" clasp
	rm -f clasp/main.cpp
    rm -f clasp/CMakeLists.txt
)

# sync libclasp
(
	cd libclasp
	svn export --force "$baseurl/libclasp/src" src
	svn export --force "$baseurl/libclasp/clasp" clasp
)
version=$(svn info "$baseurl" | grep "Last Changed Rev:" | colrm 1 18)
#sed -i "s/\\\$Revision\\\$/$version/" libclasp/clasp/clasp_facade.h
sed -i "s/#define CLASP_VERSION \"\(.*\)\"/#define CLASP_VERSION \"\1-R$version\"/g" libclasp/clasp/clasp_facade.h

