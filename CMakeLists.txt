cmake_minimum_required(VERSION 3.1)
project(CLINGO VERSION 5.1.1 LANGUAGES C CXX)
if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLINGO_LIB_SHARED "build clingo library shared" ON)
option(CLINGO_ENABLE_PYTHON "enable python support" ON)
option(CLINGO_ENABLE_LUA "enable lua support" ON)

enable_testing()

# NOTE: assumes that submodule has been initialized
add_subdirectory(clasp)

if (CLINGO_ENABLE_PYTHON)
    find_package(PythonLibs REQUIRED)
endif()
if (CLINGO_ENABLE_LUA)
    find_package(Lua REQUIRED)
endif()
find_package(BISON REQUIRED)
if (BISON_VERSION VERSION_LESS "2.5")
    message(FATAL_ERROR "at least bison version 2.5 is required")
endif()
find_package(RE2C REQUIRED)

add_subdirectory(libreify)
add_subdirectory(app/reify)
add_subdirectory(libgringo)
add_subdirectory(app/gringo)
add_subdirectory(libclingo)
add_subdirectory(app/clingo)
add_subdirectory(examples/c)
add_subdirectory(examples/cc)
if (CLINGO_ENABLE_PYTHON)
    add_subdirectory(app/pyclingo)
endif()
if (CLINGO_ENABLE_LUA)
    add_subdirectory(app/luaclingo)
endif()
