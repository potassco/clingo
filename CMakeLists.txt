cmake_minimum_required(VERSION 3.22.1)

project(PARSERTEST VERSION "1.0" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PARSER_BUILD_WEB "compile to javascript with emscripten" OFF)
option(PARSER_BUILD_TESTS "enable tests" ON)


if (PARSER_BUILD_WEB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -sMODULARIZE")
    set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
    set(DCMAKE_EXE_LINKER_FLAGS_RELEASE "")
endif()

if (PARSER_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(third_party)
add_subdirectory(lib)
if (NOT PARSER_BUILD_WEB)
    add_subdirectory(examples/c-api)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experiment.cc")
    add_executable(experiment experiment.cc)
    target_link_libraries(experiment parsertest hopscotch_map foonathan::lexy::core foonathan::lexy::file imath::imath)
endif()

if (PARSER_BUILD_WEB)
    add_subdirectory(web)
else()
    add_subdirectory(app)
endif()
