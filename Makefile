all:
	cmake -S. -Bbuild
	cmake --build build --target $@ --parallel

%:
	cmake -S. -Bbuild
	cmake --build build --target $@ --parallel

