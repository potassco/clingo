all: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake

build/cmake:
	mkdir -p build/cmake
	current="$$(pwd -P)" && cd build/cmake && cd "$$(pwd -P)" && cmake "$${current}"

.DEFAULT: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake $@

test: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake
	$(MAKE) -C build/cmake $@ CTEST_OUTPUT_ON_FAILURE=TRUE

LUA_DIR=$(HOME)/local/lua-5.1.5-js

web:
	@echo "Warning: for this target to work, resource files have to be disabled in emscripten's toolchain file!"
	mkdir -p build/web
	current="$$(pwd -P)" && cd build/web && cd "$$(pwd -P)" && emsdk construct_env && source emsdk_set_env.sh && emcmake cmake \
		-DCLINGO_ENABLE_WEB=On \
		-DCLINGO_ENABLE_PYTHON=Off \
		-DCLINGO_ENABLE_LUA=On \
		-DCLASP_BUILD_TEST=Off \
		-DBUILD_CLINGO_LIB_SHARED=Off \
		-DCLASP_BUILD_WITH_THREADS=Off \
		-DCMAKE_VERBOSE_MAKEFILE=On \
		-DCMAKE_BUILD_TYPE=release \
		-DCMAKE_CXX_FLAGS="-std=c++11 -s DISABLE_EXCEPTION_CATCHING=0" \
		-DCMAKE_CXX_FLAGS_RELEASE="-Os -Wall" \
		-DCMAKE_EXE_LINKER_FLAGS="" \
		-DCMAKE_EXE_LINKER_FLAGS_RELEASE="" \
		-DLUA_LIBRARIES=$(LUA_DIR)/lib/liblua.a \
		-DLUA_INCLUDE_DIR=$(LUA_DIR)/include \
		"$${current}"
	$(MAKE) -C build/web web
