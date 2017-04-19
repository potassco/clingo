LUA_DIR=$(HOME)/local/lua-5.1.5-js
BUILD_TYPE=debug
CC=/usr/bin/cc
CXX=/usr/bin/c++

all: build/$(BUILD_TYPE)
	cd build/$(BUILD_TYPE) && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/$(BUILD_TYPE)

build/$(BUILD_TYPE):
	mkdir -p build/$(BUILD_TYPE)
	current="$$(pwd -P)" && cd build/$(BUILD_TYPE) && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_VERBOSE_MAKEFILE=On \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_CXX_COMPILER="${CXX}" \
		-DCLINGO_BUILD_TESTS=On \
		-DCLASP_BUILD_TESTS=On \
		-DLIB_POTASSCO_BUILD_TESTS=On \
		-DCLINGO_BUILD_EXAMPLES=On \
		"$${current}"

%:: build/$(BUILD_TYPE) FORCE
	cd build/$(BUILD_TYPE) && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/$(BUILD_TYPE) $@

test: build/$(BUILD_TYPE)
	cd build/$(BUILD_TYPE) && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/$(BUILD_TYPE)
	$(MAKE) -C build/$(BUILD_TYPE) $@ CTEST_OUTPUT_ON_FAILURE=TRUE

web:
	mkdir -p build/web
	current="$$(pwd -P)" && cd build/web && cd "$$(pwd -P)" && source $$(which emsdk_env.sh) && emcmake cmake \
		-DCLINGO_BUILD_WEB=On \
		-DCLINGO_BUILD_WITH_PYTHON=Off \
		-DCLINGO_BUILD_WITH_LUA=On \
		-DCLINGO_BUILD_SHARED=Off \
		-DCLASP_BUILD_WITH_THREADS=Off \
		-DCMAKE_VERBOSE_MAKEFILE=On \
		-DCMAKE_BUILD_TYPE=release \
		-DCMAKE_CXX_FLAGS="-std=c++11 -Wall -s DISABLE_EXCEPTION_CATCHING=0" \
		-DCMAKE_CXX_FLAGS_RELEASE="-Os -DNDEBUG" \
		-DCMAKE_EXE_LINKER_FLAGS="" \
		-DCMAKE_EXE_LINKER_FLAGS_RELEASE="" \
		-DLUA_LIBRARIES=$(LUA_DIR)/lib/liblua.a \
		-DLUA_INCLUDE_DIR=$(LUA_DIR)/include \
		"$${current}"
	$(MAKE) -C build/web web

glob:
	find app libclingo libgringo libreify libluaclingo libpyclingo -name CMakeLists.txt | xargs ./cmake/glob-paths.py

FORCE:

.PHONY: all test web glob FORCE
