mkdir build

if not defined CC set "CC=%BUILD_PREFIX%\Scripts\cl.exe"
if not defined CXX set "CXX=%BUILD_PREFIX%\Scripts\cl.exe"

cmake -G "Visual Studio 17 2022" -A x64 -H. -Bbuild ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%" ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DPython_EXECUTABLE="%PYTHON%" ^
    -DCLINGO_BUILD_WITH_PYTHON=ON ^
    -DCLINGO_BUILD_WITH_LUA=OFF ^
    -DCLINGO_MANAGE_RPATH=OFF ^
    -DPYCLINGO_INSTALL="system"

cmake --build build --config Release
cmake --build build --config Release --target install
