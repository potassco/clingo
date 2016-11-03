all: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake

build/cmake:
	mkdir -p build/cmake
	current="$$(pwd -P)" && cd build/cmake && cd "$$(pwd -P)" && cmake "$${current}"

.DEFAULT: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake $@

test: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake
	$(MAKE) -C build/cmake $@ CTEST_OUTPUT_ON_FAILURE=TRUE
