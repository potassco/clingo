SHELL := /bin/zsh

all: configure
	mkdir -p build/debug
	$(MAKE) -C build/debug $@

doc:
	cd doc && doxygen

test_doc: doc
	python -m http.server --directory=doc/html

test: all
	$(MAKE) CTEST_OUTPUT_ON_FAILURE=1 -C build/debug $@

compdb: all
	compdb -p "build/debug" list -1 > compile_commands.json

build/debug/CMakeCache.txt:
	@$(MAKE) -C . reconfigure

configure: build/debug/CMakeCache.txt

reconfigure:
	@[ -z ${CONDA_PREFIX+x} ] || $(MAKE) -C . reconfigure-conda
	@[ ! -z ${CONDA_PREFIX+x} ] || $(MAKE) -C . reconfigure-default

reconfigure-default:
	cmake -S. -Bbuild/debug \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DPARSER_BUILD_TESTS=On

reconfigure-clang:
	cmake -S. -Bbuild/debug \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic -D_LIBCPP_ENABLE_ASSERTIONS" \
		-DPARSER_BUILD_TESTS=On

reconfigure-gcc:
	cmake -S. -Bbuild/debug \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="g++" \
		-DCMAKE_C_COMPILER="gcc" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_BACKTRACE" \
		-DPARSER_BUILD_TESTS=On

reconfigure-conda:
	cmake -S. -Bbuild/debug \
		-DCMAKE_BUILD_TYPE="Debug" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS="On" \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++" \
		-DPARSER_BUILD_TESTS=On

reconfigure-iwyn:
	cmake -S. -Bbuild/debug \
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
	mkdir -p build/release
	current="$$(pwd -P)" && cd build/release && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build/release
	$(MAKE) -C build/release test

profile:
	mkdir -p build/profile
	current="$$(pwd -P)" && cd build/profile && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DPARSER_PROFILE=ON \
		"$${current}"
	$(MAKE) -C build/profile
	$(MAKE) -C build/profile test

release_clang:
	mkdir -p build/release_clang
	current="$$(pwd -P)" && cd build/release_clang && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L${CONDA_PREFIX}/lib" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build/release_clang
	$(MAKE) -C build/release_clang test

web:
	mkdir -p build/web
	current="$$(pwd -P)" && cd build/web && cd "$$(pwd -P)" && source emsdk_env.sh && emcmake cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DCMAKE_EXE_LINKER_FLAGS="" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DPARSER_BUILD_TESTS=On \
		-DPARSER_BUILD_WEB=On \
		"$${current}"
	$(MAKE) -C build/web
	$(MAKE) -C build/web test

gen:
	PYTHONPATH=build/debug/lib/python-api python scripts/generate.py > lib/python-api/src/ast.cc

format_yaml:
	PYTHONPATH=build/debug/lib/python-api python scripts/format_yaml.py

venv: SHELL:=/bin/bash
venv:
	python -m venv .venv
	source .venv/bin/activate && pip install pynvim pyyaml jinja2 mypy pybind11-stubgen
	ln -rft .venv/lib/python*/site-packages -s build/debug/lib/python-api/clingo.*.so

stubs: SHELL:=/bin/bash
stubs:
	PYTHONPATH=build/debug/lib/python-api python scripts/stubs.py

%: configure
	cmake --build build/debug --target $@ --parallel

.PHONY: all doc test compdb configure reconfigure format web
