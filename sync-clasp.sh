#!/bin/bash
pushd .
tmpdir=tmp_clasp
git clone https://github.com/potassco/clasp.git  --branch 3.2.x --single-branch $tmpdir
cd $tmpdir
commit=$(git log -1 --format=%h)

# program options
cp -r libprogram_opts/src          ../libprogram_opts
cp -r libprogram_opts/program_opts ../libprogram_opts

# libclasp
cp -r libclasp/src   ../libclasp/
cp -r libclasp/clasp ../libclasp/

# libclasp
cp -r liblp/potassco ../liblp
cp -r liblp/src      ../liblp
cp -r liblp/tests    ../liblp

# applications
cp    app/clasp_app.* ../app/clingo/src/clasp
cp -r app/lpconvert   ../app/

popd

sed -i "s/#define CLASP_VERSION \"\(.*\)\"/#define CLASP_VERSION \"\1-R$commit\"/g" libclasp/clasp/clasp_facade.h
rm -rf $tmpdir

