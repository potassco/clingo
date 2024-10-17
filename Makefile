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

debug:
	mkdir -p build/$@
	cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=debug \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@

release:
	mkdir -p build/$@
	cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

release_lto:
	mkdir -p build/$@
	cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-flto=auto -fuse-linker-plugin -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-flto=auto -fuse-linker-plugin -Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

release_clang:
	mkdir -p build/$@
	cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

release_clang_lto:
	mkdir -p build/$@
	cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="clang++" \
		-DCMAKE_C_COMPILER="clang" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -flto -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-flto -Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

$@:
	mkdir -p build/$@
	cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DPARSER_PROFILE=ON \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

web:
	mkdir -p build/$@
	. emsdk_env.sh && emcmake cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DPARSER_BUILD_TESTS=On \
		-DPARSER_BUILD_WEB=On \
		-DCMAKE_EXE_LINKER_FLAGS="" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

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
	source .venv/bin/activate && PYTHONPATH=build/debug/lib/python-api python scripts/stubs.py

.PHONY: all doc test compdb stubs venv gen format_yaml debug release release_lto release_clang release_clang_lto web
