mkdir build

cmake -G "%CMAKE_GENERATOR%" -H. -Bbuild ^
    -DCMAKE_CXX_COMPILER="%CXX%" ^
    -DCMAKE_C_COMPILER="%CC%" ^
    -DPython_ROOT_DIR="%PREFIX%" ^
    -DPYTHON_EXECUTABLE="%PYTHON%" ^
    -DCLINGO_REQUIRE_PYTHON=ON ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%" ^
    -DPYCLINGO_USER_INSTALL=OFF ^
    -DCLINGO_BUILD_WITH_LUA=OFF ^
    -DCLINGO_MANAGE_RPATH=OFF

cmake --build build --config Release
cmake --build build --config Release --target install
