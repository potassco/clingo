cmake -G "Visual Studio 17 2022" -S . -B build ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DPython_EXECUTABLE="%PYTHON%" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%" ^
    -DCLINGO_MANAGE_RPATH=OFF ^
    -DCMAKE_INSTALL_LIBDIR="lib"

cmake --build build --config Release
cmake --build build --target install --config Release
