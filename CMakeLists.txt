cmake_minimum_required(VERSION 3.22.1)

project(CLINGO VERSION "1.0" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PARSER_BUILD_WEB "compile to javascript with emscripten" OFF)
option(PARSER_BUILD_TESTS "enable tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(macros)

find_package(RE2C "2.0" REQUIRED)

if (PARSER_BUILD_WEB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ENVIRONMENT=worker,node -s ALLOW_MEMORY_GROWTH=1 -s INVOKE_RUN=1")
    set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
    set(DCMAKE_EXE_LINKER_FLAGS_RELEASE "")
endif()

if (PARSER_BUILD_TESTS)
    set(LIB_POTASSCO_SETUP_CATCH2 TRUE)
    enable_testing()
endif()

if (PARSER_PROFILE)
    find_package(Gperftools REQUIRED)
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

add_subdirectory(app)
if (PARSER_BUILD_WEB)
    add_subdirectory(web)
endif()
