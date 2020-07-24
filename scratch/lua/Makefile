VERSION=5.3.5
SHELL=/bin/bash

all: source/CMakeLists.txt
	source "$$(which emsdk)_env.sh" && emcmake cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=${HOME}/local/opt/lua-js -Hsource -Bbuild
	$(MAKE) -C build
	$(MAKE) -C build install

lua.tar.gz:
	wget -O lua.tar.gz https://www.lua.org/ftp/lua-$(VERSION).tar.gz

source: lua.tar.gz
	mkdir -p source
	tar xf lua.tar.gz -C source --strip-components 1

source/CMakeLists.txt: source
	cp CMakeLists.txt source/CMakeLists.txt

clean:
	rm -rf build source lua.tar.gz

.PHONY: all clean
