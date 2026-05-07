cmake -S . -B build -G "Visual Studio 18 2026" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%" ^
    -DCMAKE_INSTALL_LIBDIR="lib" ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DPython_EXECUTABLE="%PYTHON%" ^
    -DCLINGO_MANAGE_RPATH=OFF ^
    -DCLINGO_BUILD_TESTS=ON ^
    -DPYCLINGO_INSTALL_DIR="%SP_DIR%"

cmake --build build --config Release
ctest --test-dir build -C Release
cmake --build build --target install --config Release
