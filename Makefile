SHELL := /bin/zsh
CPU_COUNT := $(shell test -e /usr/bin/nproc && nproc || echo "1")

LDFLAGS :=
CFLAGS := -Wall -Wextra -pedantic -march=native -mtune=native
CXXFLAGS := $(CFLAGS)
CTEST_FLAGS := CTEST_OUTPUT_ON_FAILURE=1 CTEST_PARALLEL_LEVEL=$(CPU_COUNT)

CLANG_LDFLAGS := $(LDFLAGS) -fuse-ld=lld
CLANG_CFLAGS := $(CFLAGS)
CLANG_CXXFLAGS := -stdlib=libc++ $(CXXFLAGS)
CLANG_CC := $(shell test -e /usr/bin/clang-20 && echo /usr/bin/clang-20 || echo clang)
CLANG_CXX := $(shell test -e /usr/bin/clang++-20 && echo /usr/bin/clang++-20 || echo clang++)
ifdef CONDA_PREFIX
  CLANG_LDFLAGS := -Wl,-rpath=$(CONDA_PREFIX)/lib -L$(CONDA_PREFIX)/lib $(CLANG_LDFLAGS)
endif

EMSCRIPTEN_LDFLAGS :=
EMSCRIPTEN_CFLAGS := -Wall -Wextra -pedantic
EMSCRIPTEN_CXXFLAGS := $(EMSCRIPTEN_CFLAGS)

OPTIONS := -DCLINGO_BUILD_TESTS=On \
-DCMAKE_C_COMPILER="$(CC)" \
-DCMAKE_CXX_COMPILER="$(CXX)" \
-DCMAKE_EXE_LINKER_FLAGS="$(LDFLAGS)" \
-DCMAKE_MODULE_LINKER_FLAGS="$(LDFLAGS)" \
-DCMAKE_SHARED_LINKER_FLAGS="$(LDFLAGS)" \
-DCMAKE_C_FLAGS="$(CFLAGS)" \
-DCMAKE_CXX_FLAGS="$(CXXFLAGS)"

CLANG_OPTIONS := -DCLINGO_BUILD_TESTS=On \
-DCMAKE_C_COMPILER="$(CLANG_CC)" \
-DCMAKE_CXX_COMPILER="$(CLANG_CXX)" \
-DCMAKE_EXE_LINKER_FLAGS="$(CLANG_LDFLAGS)" \
-DCMAKE_MODULE_LINKER_FLAGS="$(CLANG_LDFLAGS)" \
-DCMAKE_SHARED_LINKER_FLAGS="$(CLANG_LDFLAGS)" \
-DCMAKE_C_FLAGS="$(CLANG_CFLAGS)" \
-DCMAKE_CXX_FLAGS="$(CLANG_CXXFLAGS)"

EMSCRIPTEN_OPTIONS := -DCLINGO_BUILD_TESTS=On \
-DCMAKE_EXE_LINKER_FLAGS="$(EMSCRIPTEN_LDFLAGS)" \
-DCMAKE_MODULE_LINKER_FLAGS="$(EMSCRIPTEN_LDFLAGS)" \
-DCMAKE_SHARED_LINKER_FLAGS="$(EMSCRIPTEN_LDFLAGS)" \
-DCMAKE_C_FLAGS="$(EMSCRIPTEN_CFLAGS)" \
-DCMAKE_CXX_FLAGS="$(EMSCRIPTEN_CXXFLAGS)"

all: debug

doc:
	cd doc && rm -rf html && doxygen

test: debug
	$(MAKE) $(CTEST_FLAGS) -C build/debug $@

.venv:
	python3 -m venv .venv
	source .venv/bin/activate && pip install isort black pynvim pyyaml jinja2 mypy pybind11-stubgen pdoc compdb

venv: .venv

compdb: .venv build/debug/CMakeCache.txt
	rm -f compile_commands.json
	source .venv/bin/activate && compdb -p "build/debug" list -1 > compile_commands.json
	source .venv/bin/activate && python "scripts/compdb-cpp-headers.py"

build/debug/CMakeCache.txt: .venv
	mkdir -p build/debug
	source .venv/bin/activate && cmake -S. -Bbuild/debug \
		-DCMAKE_BUILD_TYPE=debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=On \
		$(CLANG_OPTIONS)

debug: build/debug/CMakeCache.txt
	$(MAKE) -C build/$@

release:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		$(OPTIONS)
	$(MAKE) -C build/$@
	$(MAKE) $(CTEST_FLAGS) -C build/$@ test

release_lto:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=On \
		$(OPTIONS)
	$(MAKE) -C build/$@
	$(MAKE) $(CTEST_FLAGS) -C build/$@ test

release_clang:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		$(CLANG_OPTIONS)
	$(MAKE) -C build/$@
	$(MAKE) $(CTEST_FLAGS) -C build/$@ test

release_clang_lto:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=On \
		$(CLANG_OPTIONS)
	$(MAKE) -C build/$@
	$(MAKE) $(CTEST_FLAGS) -C build/$@ test

profile:
	mkdir -p build/$@
	source .venv/bin/activate && cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DCLINGO_PROFILE=ON \
		$(CLANG_OPTIONS)
	$(MAKE) -C build/$@
	$(MAKE) $(CTEST_FLAGS) -C build/$@ test

web:
	mkdir -p build/$@
	. emsdk_env.sh && emcmake cmake -S. -Bbuild/$@ \
		-DCMAKE_BUILD_TYPE=release \
		-DCLINGO_BUILD_WEB=On \
		$(EMSCRIPTEN_OPTIONS)
	$(MAKE) -C build/$@
	$(MAKE) $(CTEST_FLAGS) -C build/$@ test

gen:
	source .venv/bin/activate && PYTHONPATH=build/debug/bin/python python scripts/generate.py > lib/python-api/src/ast.cc

format_yaml:
	source .venv/bin/activate && PYTHONPATH=build/debug/bin/python python scripts/format_yaml.py

stubs: debug
	source .venv/bin/activate && python scripts/stubs.py

pdoc: debug venv
	source .venv/bin/activate && python scripts/stubs.py --python
	cd pdoc && python3 -m venv .venv
	cd pdoc && source .venv/bin/activate && pip install pdoc typing_extensions
	cd pdoc && source .venv/bin/activate && rm -rf html && pdoc -o html --no-show-source -d google ./clingo

.PHONY: all doc test compdb stubs pdoc venv debug gen format_yaml debug release release_lto release_clang release_clang_lto web
