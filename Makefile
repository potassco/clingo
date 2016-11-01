all: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake

build/cmake:
	mkdir -p build/cmake
	current="$$(pwd -P)" && cd build/cmake && cd "$$(pwd -P)" && cmake "$${current}"

.DEFAULT: build/cmake
	cd build/cmake && cd $$(pwd -P) && cmake .
	$(MAKE) -C build/cmake $@
