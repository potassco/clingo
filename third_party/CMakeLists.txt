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

if (NOT CLINGO_BUILD_WEB AND NOT DEFINED SKBUILD)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pytest-cmake/cmake")
    find_package(Pytest 8.0.0)
    add_subdirectory(pybind11)
endif()

add_subdirectory(clasp)
set_property(TARGET libclasp PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libpotassco PROPERTY POSITION_INDEPENDENT_CODE ON)

clingo_target_properties(TARGETS libclasp libpotassco potassco_default_warnings imath ordered_map hopscotch_map TYPE extra)
