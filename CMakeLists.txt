cmake_minimum_required(VERSION 3.1)

project(PARSERTEST VERSION "1.0" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PARSER_BUILD_WEB "compile to javascript with emscripten" OFF)
option(PARSER_BUILD_TESTS "enable tests" ON)


if (PARSER_BUILD_WEB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -sMODULARIZE")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
    set(DCMAKE_EXE_LINKER_FLAGS_RELEASE "")
endif()


if (PARSER_BUILD_TESTS)
    set(CATCH_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "" FORCE)
    enable_testing()
    add_subdirectory(third_party/Catch2)
endif()
add_subdirectory(third_party/lexy)
add_subdirectory(lib)

if (PARSER_BUILD_WEB)
    add_subdirectory(web)
else()
    add_subdirectory(app)
endif()
