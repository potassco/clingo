mkdir build

cmake -G "%CMAKE_GENERATOR%" -H. -Bbuild ^
    -DCMAKE_CXX_COMPILER="%CXX%" ^
    -DCMAKE_C_COMPILER="%CC%" ^
    -DPYTHON_EXECUTABLE="%PYTHON%" ^
    -DCLINGO_REQUIRE_PYTHON=ON ^
    -DCMAKE_INSTALL_PREFIX="%PREFIX%" ^
    -DPYCLINGO_USER_INSTALL=OFF ^
    -DCLINGO_BUILD_WITH_LUA=OFF ^
    -DCLINGO_MANAGE_RPATH=OFF ^
    -DCMAKE_INSTALL_BINDIR="."

cmake --build build --config Release
cmake --build build --config Release --target install
