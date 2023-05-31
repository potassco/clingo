SHELL := /bin/zsh

all: configure
	cmake --build build --target $@ --parallel

test: all
	cmake --build build --target $@ --parallel

compdb: all
	compdb -p "build" list -1 > compile_commands.json

build/CMakeCache.txt:
	$(MAKE) -C . reconfigure

configure: build/CMakeCache.txt

reconfigure:
	[ ! -z "${CONDA_PREFIX+x}" ] || cmake -S. -Bbuild \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -ftemplate-backtrace-limit=0"
	[ -z "${CONDA_PREFIX+x}" ] || cmake -S. -Bbuild \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On"

reconfigure-iwyn:
	cmake -S. -Bbuild \
		-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="include-what-you-use;-w;-Xiwyu" \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++"

format:
	clang-tidy --verify-config
	clang-tidy -fix {app,lib,tests}/**/*{.cc,.hh}(N)
	clang-format -i {app,lib,tests}/**/*{.cc,.hh}(N)

Makefile:
	@:

%: configure
	cmake --build build --target $@ --parallel

.PHONY: all test compdb configure reconfigure format

