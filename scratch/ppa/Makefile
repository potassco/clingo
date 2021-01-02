VERSION=$(shell head -n 1 clingo/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')

sync:
	rsync -av --exclude __pycache__ ../../app ../../clasp ../../libgringo ../../libclingo ../../libluaclingo ../../libpyclingo ../../libpyclingo_cffi ../../libreify ../../cmake ../../CMakeLists.txt ../../CHANGES.md ../../README.md ../../INSTALL.md ../../LICENSE.md clingo/
source: sync
	cd clingo && debuild --no-tgz-check -S
deb: sync
	cd clingo && debuild --no-tgz-check
put: source
	dput ppa:potassco/wip clingo_$(VERSION)_source.changes
clean:
	rm -rf clingo/app clingo/clasp clingo/libgringo clingo/libclingo clingo/libluaclingo clingo/libpyclingo clingo/libpyclingo_cffi clingo/libreify clingo/cmake clingo/CMakeLists.txt clingo/CHANGES.md clingo/README.md clingo/INSTALL.md clingo/LICENSE.md
	rm -rf *.build *.deb *.dsc *.buildinfo *.changes *.ddeb *.tar.xz *.upload
	rm -rf clingo/debian/files clingo/debian/.debhelper/ clingo/debian/clingo.debhelper.log clingo/debian/clingo.substvars clingo/debian/clingo/ clingo/debian/debhelper-build-stamp clingo/debian/libclingo-dev.debhelper.log clingo/debian/libclingo-dev.substvars clingo/debian/libclingo-dev/ clingo/debian/libclingo.debhelper.log clingo/debian/libclingo.substvars clingo/debian/libclingo/ clingo/debian/python3-clingo.debhelper.log clingo/debian/python3-clingo.postinst.debhelper clingo/debian/python3-clingo.prerm.debhelper clingo/debian/python3-clingo.substvars clingo/debian/python3-clingo/ clingo/debian/tmp/ clingo/obj-x86_64-linux-gnu/
