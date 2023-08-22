SHELL := /bin/zsh

all: configure
	$(MAKE) -C build $@

test: all
	$(MAKE) CTEST_OUTPUT_ON_FAILURE=1 -C build $@

compdb: all
	compdb -p "build" list -1 > compile_commands.json

build/CMakeCache.txt:
	@$(MAKE) -C . reconfigure

configure: build/CMakeCache.txt

reconfigure:
	@[ -z ${CONDA_PREFIX+x} ] || $(MAKE) -C . reconfigure-conda
	@[ ! -z ${CONDA_PREFIX+x} ] || $(MAKE) -C . reconfigure-default

reconfigure-default:
	cmake -S. -Bbuild \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DPARSER_BUILD_TESTS=On

reconfigure-clang:
	cmake -S. -Bbuild \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic -fsanitize=undefined -D_LIBCPP_ENABLE_ASSERTIONS" \
		-DPARSER_BUILD_TESTS=On

reconfigure-gcc:
	cmake -S. -Bbuild \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="g++" \
		-DCMAKE_C_COMPILER="gcc" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_BACKTRACE" \
		-DPARSER_BUILD_TESTS=On

reconfigure-conda:
	cmake -S. -Bbuild \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++" \
		-DPARSER_BUILD_TESTS=On

reconfigure-iwyn:
	cmake -S. -Bbuild \
		-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="include-what-you-use;-w;-Xiwyu" \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++" \
		-DPARSER_BUILD_TESTS=On

format:
	clang-tidy --verify-config
	clang-tidy -fix {app,lib,tests}/**/*{.cc,.hh}(N)
	clang-format -i {app,lib,tests}/**/*{.cc,.hh}(N)

Makefile:
	@:

release:
	mkdir -p build_release
	current="$$(pwd -P)" && cd build_release && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build_release
	$(MAKE) -C build_release test

release_clang:
	mkdir -p build_release_clang
	current="$$(pwd -P)" && cd build_release_clang && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build_release_clang
	$(MAKE) -C build_release_clang test

web:
	mkdir -p build_web
	current="$$(pwd -P)" && cd build_web && cd "$$(pwd -P)" && source emsdk_env.sh && emcmake cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DPARSER_BUILD_TESTS=On \
		-DPARSER_BUILD_WEB=On \
		"$${current}"
	$(MAKE) -C build_web
	$(MAKE) -C build_web test

%: configure
	cmake --build build --target $@ --parallel

.PHONY: all test compdb configure reconfigure format web

