SHELL := /bin/zsh

all: debug

doc:
	cd doc && doxygen

test_doc: doc
	python3 -m http.server --directory=doc/html

test: debug
	$(MAKE) CTEST_OUTPUT_ON_FAILURE=1 -C build/debug $@

compdb: all
	compdb -p "build/debug" list -1 > compile_commands.json
	python3 "scripts/compdb-cpp-headers.py"

build/debug/CMakeCache.txt:
	@$(MAKE) -C . reconfigure

Makefile:
	@:

debug:
	mkdir -p build/debug
	cmake -S. -Bbuild/debug \
		-DCMAKE_BUILD_TYPE=debug \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/debug

release:
	mkdir -p build/release
	current="$$(pwd -P)" && cd build/release && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build/release
	$(MAKE) -C build/release test

release_lto:
	mkdir -p build/release_lto
	current="$$(pwd -P)" && cd build/release_lto && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-flto=auto -fuse-linker-plugin -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-flto=auto -fuse-linker-plugin -Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build/release_lto
	$(MAKE) -C build/release_lto test

release_clang:
	mkdir -p build/release_clang
	current="$$(pwd -P)" && cd build/release_clang && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build/release_clang
	$(MAKE) -C build/release_clang test

release_clang_lto:
	mkdir -p build/release_clang_lto
	current="$$(pwd -P)" && cd build/release_clang_lto && cd "$$(pwd -P)" && cmake \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -flto -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-flto -Wall -Wextra -pedantic" \
		"$${current}"
	$(MAKE) -C build/release_clang_lto
	$(MAKE) -C build/release_clang_lto test

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
	python3 -m venv .venv
	source .venv/bin/activate && pip install pynvim pyyaml jinja2 mypy pybind11-stubgen
	ln -rft .venv/lib/python*/site-packages -s build/debug/lib/python-api/clingo.*.so

stubs: SHELL:=/bin/bash
stubs:
	PYTHONPATH=build/debug/lib/python-api python scripts/stubs.py

.PHONY: all doc test compdb configure format web
