cmake_minimum_required(VERSION 3.22.1)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/lib/c-api/include/clingo/core.h" CLINGO_VERSION_LINE REGEX "#define CLINGO_VERSION ")
string(REGEX REPLACE "#define CLINGO_VERSION[ \t]+\"([^\"]+)\"" "\\1" CLINGO_VERSION "${CLINGO_VERSION_LINE}")

project(CLINGO VERSION "${CLINGO_VERSION}" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(macros)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(CLINGO_BUILD_WEB "compile to javascript with emscripten" OFF)
option(CLINGO_MANAGE_RPATH "manage rpath when installing" ON)
option(CLINGO_PROFILE "configure profiling" OFF)
option(CLINGO_BUILD_TESTS "enable tests" ON)
option(CLINGO_BUILD_APP "build application" ON)
option(CLINGO_BUILD_EXAMPLES "build examples" ON)

cmake_dependent_option(CLINGO_BUILD_SHARED "build clingo as shared library" ON "NOT EMSCRIPTEN" OFF)
cmake_dependent_option(CLINGO_USE_VERSION_SCRIPT "use a version script to hide C++ symbols" ON "CLINGO_BUILD_SHARED" OFF)
cmake_dependent_option(CLINGO_USE_VISIBILITY "enable visibility attributes" ON "CLINGO_BUILD_SHARED" OFF)

set(PYCLINGO_INSTALL "prefix" CACHE STRING "where to install python module: user, system, prefix")
set(PYCLINGO_INSTALL_DIR "" CACHE STRING "configure where to install the python module.")

if(${CMAKE_VERSION} VERSION_LESS "3.26")
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if (CLINGO_USE_VISIBILITY)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

if (CLINGO_BUILD_WEB)
    message(STATUS "Configuring for web")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sENVIRONMENT=worker,node -sALLOW_MEMORY_GROWTH=1 -sINVOKE_RUN=1")
    set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
    set(DCMAKE_EXE_LINKER_FLAGS_RELEASE "")
endif()

set(PYBIND11_NEWPYTHON ON)
set(PYBIND11_FINDPYTHON ON)
set(CLINGO_INSTALL_DEFAULT OFF)
set(CLINGO_INSTALL_EXTRA OFF)
set(CLINGO_INSTALL_WHEEL OFF)

if (DEFINED SKBUILD)
    message(STATUS "Configuring for skbuild")
    set(CLASP_BUILD_APP OFF)
    set(CLINGO_BUILD_TESTS OFF)
    set(CLINGO_BUILD_WEB OFF)
    set(CLINGO_BUILD_APP OFF)
    set(CLINGO_BUILD_EXAMPLES OFF)
    set(CLINGO_INSTALL_WHEEL ON)
    find_package(pybind11 CONFIG REQUIRED)
    if (EMSCRIPTEN)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -sSIDE_MODULE=1")
    endif()
else()
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSystemDir)
    if(CLINGO_MANAGE_RPATH AND isSystemDir EQUAL -1)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()
    set(CLINGO_INSTALL_DEFAULT ON)
    if (NOT CLINGO_BUILD_SHARED)
        set(CLINGO_INSTALL_EXTRA ON)
    endif()
    if (NOT CLINGO_BUILD_WEB)
        find_package(Python COMPONENTS Interpreter Development Development.Module Development.Embed)
    endif()
endif()

if (EMSCRIPTEN)
    message(STATUS "Configuring for esmcripten")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -sDISABLE_EXCEPTION_CATCHING=0")
endif()

if (CLINGO_BUILD_TESTS)
    set(LIB_POTASSCO_SETUP_CATCH2 TRUE)
    enable_testing()
endif()

if (CLINGO_PROFILE)
    find_package(Gperftools REQUIRED)
endif()

if (MSVC)
    add_compile_options(/bigobj)
endif()

find_package(RE2C "2.0" REQUIRED)

add_subdirectory(third_party)
add_subdirectory(lib libclingo)
if (CLINGO_BUILD_EXAMPLES)
    add_subdirectory(examples/c-api)
endif()
if (CLINGO_BUILD_APP)
    add_subdirectory(app)
endif()
if (CLINGO_BUILD_WEB)
    add_subdirectory(web)
endif()

if (CLINGO_INSTALL_DEFAULT)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clingo-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/clingo-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/clingo"
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/clingo-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/clingo-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/clingo-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/clingo"
    )
    install(EXPORT clingo-targets
        FILE clingo-targets.cmake
        NAMESPACE clingo::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/clingo"
    )
endif()
