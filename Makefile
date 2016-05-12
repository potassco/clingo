target=gringo-app
cmake_options=-DWITH_LUA=shipped -DWITH_LUASQL=0

all: release

debug:
	mkdir -p build/debug
	cd build/debug && \
		cmake ../.. \
		-DCMAKE_CXX_FLAGS="-W -Wall -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC" \
		-DCMAKE_BUILD_TYPE=debug \
		${cmake_options} && \
	$(MAKE) $(target)

release:
	mkdir -p build/release
	cd build/release && \
	cmake ../.. \
		-DCMAKE_CXX_FLAGS=-Wall \
		-DCMAKE_BUILD_TYPE=release \
		${cmake_options} && \
	$(MAKE) $(target)

static:
	mkdir -p build/static
	cd build/static && \
	cmake ../.. \
		-DCMAKE_CXX_FLAGS=-Wall \
		-DCMAKE_BUILD_TYPE=release \
		-DUSE_STATIC_LIBS=ON \
		${cmake_options} && \
	$(MAKE) $(target)

static32:
	mkdir -p build/static32
	cd build/static32 && \
	cmake ../.. \
		-DCMAKE_CXX_FLAGS="-Wall -m32" \
		-DCMAKE_FIND_ROOT_PATH="/usr/lib32;/home/wv/bin/linux/32/boost-1.44" \
		-DCMAKE_BUILD_TYPE=release \
		-DUSE_STATIC_LIBS=ON \
		${cmake_options} && \
	$(MAKE) $(target)

mingw32:
	@[ -x build/release/bin/lemon ] || (echo "error: make release first" && false)
	mkdir -p build/mingw32/bin
	cd build/mingw32 && \
	cmake ../.. \
		-DIMPORT_LEMON="$(PWD)/build/release/import_lemon.cmake" \
		-DCMAKE_TOOLCHAIN_FILE=../../cmake/mingw32.cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DUSE_STATIC_LIBS=ON \
		${cmake_options} && \
	$(MAKE) $(target)

clean:
	rm -rf build
