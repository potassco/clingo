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
	cmake -S. -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
		-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="include-what-you-use;-w;-Xiwyu" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++"

reconfigure-iwyn:
	cmake -S. -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
		-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="include-what-you-use;-w;-Xiwyu" \
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

