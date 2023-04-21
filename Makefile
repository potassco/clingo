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
	cmake -S. -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=On

Makefile:
	@:

%: configure
	cmake --build build --target $@ --parallel

.PHONY: all test compdb configure reconfigure

