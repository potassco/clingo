all:
	cmake -S. -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=On
	cmake --build build --target $@ --parallel

test: all
	cmake --build build --target $@ --parallel

compdb: all
	compdb -p "build" list -1 > compile_commands.json

%:
	cmake -S. -Bbuild
	cmake --build build --target $@ --parallel

.PHONY: all test compdb

