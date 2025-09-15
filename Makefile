SHELL := /bin/zsh
CLANG_CC := $(shell test -e /usr/bin/clang-20 && echo /usr/bin/clang-20 || echo clang)
CLANG_CXX := $(shell test -e /usr/bin/clang++-20 && echo /usr/bin/clang++-20 || echo clang++)
CPU_COUNT := $(shell test -e /usr/bin/nproc && nproc || echo "1")

all: debug

doc:
	cd doc && rm -rf html && doxygen

test: debug
	$(MAKE) CTEST_OUTPUT_ON_FAILURE=1 CTEST_PARALLEL_LEVEL=$(CPU_COUNT) -C build/debug $@

.venv:
	python3 -m venv .venv
	source .venv/bin/activate && pip install isort black pynvim pyyaml jinja2 mypy pybind11-stubgen pdoc compdb

venv: .venv

compdb: .venv build/debug/CMakeCache.txt
	source .venv/bin/activate && compdb -p "build/debug" list -1 > compile_commands.json
	source .venv/bin/activate && python "scripts/compdb-cpp-headers.py"

build/debug/CMakeCache.txt: .venv
	mkdir -p build/debug
	source .venv/bin/activate && cmake -S. -Bbuild/debug \
		-DCMAKE_CXX_COMPILER="$(CLANG_CXX)" \
		-DCMAKE_C_COMPILER="$(CLANG_CC)" \
		-DCMAKE_BUILD_TYPE=debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=On \
		-DCLINGO_BUILD_TESTS=On \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"

debug: build/debug/CMakeCache.txt
	$(MAKE) -C build/$@

release:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

release_lto:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_TESTS=On \
		-DCMAKE_CXX_FLAGS="-flto=auto -fuse-linker-plugin -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-flto=auto -fuse-linker-plugin -Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

release_clang:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="$(CLANG_CXX)" \
		-DCMAKE_C_COMPILER="$(CLANG_CC)" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

release_clang_lto:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_TESTS=On \
		-DCMAKE_CXX_COMPILER="$(CLANG_CXX)" \
		-DCMAKE_C_COMPILER="$(CLANG_CC)" \
		-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
		-DCMAKE_CXX_FLAGS="-stdlib=libc++ -flto -Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-flto -Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

profile:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_TESTS=On \
		-DCLINGO_PROFILE=ON \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

web:
	mkdir -p build/$@
	. emsdk_env.sh && emcmake cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_TESTS=On \
		-DCLINGO_BUILD_WEB=On \
		-DCMAKE_EXE_LINKER_FLAGS="" \
		-DCMAKE_C_FLAGS="-Wall -Wextra -pedantic" \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -pedantic"
	$(MAKE) -C build/$@
	$(MAKE) -C build/$@ test

gen:
	source .venv/bin/activate && PYTHONPATH=build/debug/bin/python python scripts/generate.py > lib/python-api/src/ast.cc

format_yaml:
	source .venv/bin/activate && PYTHONPATH=build/debug/bin/python python scripts/format_yaml.py

stubs: debug
	source .venv/bin/activate && python scripts/stubs.py

pdoc: debug venv
	source .venv/bin/activate && python scripts/stubs.py --python
	cd pdoc && python3 -m venv .venv
	cd pdoc && source .venv/bin/activate && pip install pdoc
	cd pdoc && source .venv/bin/activate && rm -rf html && pdoc -o html --no-show-source -d google ./clingo

.PHONY: all doc test compdb stubs pdoc venv debug gen format_yaml debug release release_lto release_clang release_clang_lto web
