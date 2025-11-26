add_library(hopscotch_map INTERFACE)
target_include_directories(hopscotch_map INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hopscotch-map/include>)
add_library(tsl::hopscotch_map ALIAS hopscotch_map)

add_library(ordered_map INTERFACE)
target_include_directories(ordered_map INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ordered-map/include>)
add_library(tsl::ordered_map ALIAS ordered_map)

add_library(imath INTERFACE)
add_library(imath::imath ALIAS imath)
target_sources(imath INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imath/imath.c>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imath/imrat.c>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imath/gmp_compat.c>")
target_include_directories(imath INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imath>")

if(CLINGO_BUILD_PYTHON)
    if(CLINGO_BUILD_TESTS)
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pytest-cmake/cmake")
        find_package(Pytest 8.0.0 REQUIRED)
    endif()
    if(NOT DEFINED SKBUILD)
        add_subdirectory(pybind11)
    endif()
endif()

# NOTE: We force disable library installation here because the clingo library
# itself does not require any of the clasp headers to be installed. When the
# shared clingo library is disabled the required clasp libraries are added to
# the clingo targets.
set(CLASP_INSTALL_LIB OFF CACHE BOOL "forced to off by clingo" FORCE)
set(LIB_POTASSCO_INSTALL_LIB OFF CACHE BOOL "forced to off by clingo" FORCE)
add_subdirectory(clasp)
set_property(TARGET libclasp PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libpotassco PROPERTY POSITION_INDEPENDENT_CODE ON)
if(CLINGO_BUILD_TESTS)
    include(Catch)
endif()

clingo_target_properties(TARGETS libclasp libpotassco potassco_default_warnings potassco_default_options imath ordered_map hopscotch_map TYPE extra)
