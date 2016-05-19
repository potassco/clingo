DEBUG_FLAGS=-W -Wall -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
RELEASE_FLAGS=-Wall -DNDEBUG

gringo_release:
	mkdir -p build/gringo/release
	cd build/gringo/release && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS)" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=gringo ../../.. && $(MAKE)

gringo_debug:
	mkdir -p build/gringo/debug
	cd build/gringo/debug && cmake -DCMAKE_CXX_FLAGS="$(DEBUG_FLAGS)" -DCMAKE_BUILD_TYPE=debug -DGRINGO_TYPE=gringo ../../.. && $(MAKE)

gringo_static:
	mkdir -p build/gringo/static
	cd build/gringo/static && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=gringo ../../.. && $(MAKE)

gringo_static32:
	mkdir -p build/gringo/static32
	cd build/gringo/static32 && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static -m32" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=gringo ../../.. && $(MAKE)

gringo_mingw32: gringo_release
	mkdir -p build/gringo/mingw32/bin
	cd build/gringo/mingw32/bin && cp -sf ../../../gringo/release/bin/lemon lemon
	cd build/gringo/mingw32 && cmake -DCMAKE_TOOLCHAIN_FILE=../../../cmake/mingw32.cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=gringo ../../.. && $(MAKE)

clingo_release:
	mkdir -p build/clingo/release
	cd build/clingo/release && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS)" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=clingo ../../.. && $(MAKE)

clingo_debug:
	mkdir -p build/clingo/debug
	cd build/clingo/debug && cmake -DCMAKE_CXX_FLAGS="$(DEBUG_FLAGS)" -DCMAKE_BUILD_TYPE=debug -DGRINGO_TYPE=clingo ../../.. && $(MAKE)

clingo_static:
	mkdir -p build/clingo/static
	cd build/clingo/static && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=clingo ../../.. && $(MAKE)

clingo_static32:
	mkdir -p build/clingo/static32
	cd build/clingo/static32 && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static -m32" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=clingo ../../.. && $(MAKE)

clingo_mingw32: gringo_release
	mkdir -p build/clingo/mingw32/bin
	cd build/clingo/mingw32/bin && cp -sf ../../../gringo/release/bin/lemon lemon
	cd build/clingo/mingw32 && cmake -DCMAKE_TOOLCHAIN_FILE=../../../cmake/mingw32.cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=clingo ../../.. && $(MAKE)

iclingo_release:
	mkdir -p build/iclingo/release
	cd build/iclingo/release && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS)" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=iclingo ../../.. && $(MAKE)

iclingo_debug:
	mkdir -p build/iclingo/debug
	cd build/iclingo/debug && cmake -DCMAKE_CXX_FLAGS="$(DEBUG_FLAGS)" -DCMAKE_BUILD_TYPE=debug -DGRINGO_TYPE=iclingo ../../.. && $(MAKE)

iclingo_static32:
	mkdir -p build/iclingo/static32
	cd build/iclingo/static32 && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static -m32" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=iclingo ../../.. && $(MAKE)

iclingo_static:
	mkdir -p build/iclingo/static
	cd build/iclingo/static && cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=iclingo ../../.. && $(MAKE)

iclingo_mingw32: gringo_release
	mkdir -p build/iclingo/mingw32/bin
	cd build/iclingo/mingw32/bin && cp -sf ../../../gringo/release/bin/lemon lemon
	cd build/iclingo/mingw32 && cmake -DCMAKE_TOOLCHAIN_FILE=../../../cmake/mingw32.cmake -DCMAKE_CXX_FLAGS="$(RELEASE_FLAGS) -static" -DCMAKE_BUILD_TYPE=release -DGRINGO_TYPE=iclingo ../../.. && $(MAKE)

all_release: gringo_release clingo_release iclingo_release

all_debug: gringo_debug clingo_debug iclingo_debug

all_static: gringo_static clingo_static iclingo_static

all_static32: gringo_static32 clingo_static32 iclingo_static32

all_mingw32: gringo_mingw32 clingo_mingw32 iclingo_mingw32

all: all_release all_debug all_static all_static32 all_mingw32

clean: 
	rm -rf build

