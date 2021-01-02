VERSION=$(shell head -n 1 clingo/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[0-9]\+\)\?')

sync:
	rsync -av ../../app ../../clasp ../../libgringo ../../libclingo ../../libluaclingo ../../libpyclingo ../../libpyclingo_cffi ../../libreify ../../cmake ../../CMakeLists.txt ../../CHANGES.md ../../README.md ../../INSTALL.md ../../LICENSE.md clingo/
source: sync
	cd clingo && debuild -S
deb: sync
	cd clingo && debuild
put: source
	dput ppa:potassco/wip clingo_$(VERSION)_source.changes
clean:
	rm -rf clingo/app clingo/clasp clingo/libgringo clingo/libclingo clingo/libluaclingo clingo/libpyclingo clingo/libpyclingo_cffi clingo/libreify clingo/cmake clingo/CMakeLists.txt clingo/CHANGES.md clingo/README.md clingo/INSTALL.md clingo/LICENSE.md
	rm -rf *.build *.deb *.dsc *.buildinfo *.changes *.ddeb *.tar.xz *.upload
