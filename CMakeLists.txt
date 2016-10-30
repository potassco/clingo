cmake_minimum_required(VERSION 3.1)
project(CLINGO VERSION 5.1.1 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLINGO_LIB_SHARED "build clingo library shared" ON)

# NOTE: assumes that submodule has been initialized
add_subdirectory(clasp)

find_package(PythonLibs)
find_package(Lua)
find_package(BISON REQUIRED)
if (BISON_VERSION VERSION_LESS "2.5")
    message(FATAL_ERROR "at least bison version 2.5 is required")
endif()
find_package(RE2C REQUIRED)

add_subdirectory(libreify)
add_subdirectory(libgringo)
add_subdirectory(libclingo)
add_subdirectory(app/clingo)
add_subdirectory(examples/c)
