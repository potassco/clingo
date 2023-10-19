add_subdirectory(lexy)

add_library(hopscotch_map INTERFACE)
target_include_directories(hopscotch_map INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hopscotch-map/include>)
add_library(tsl::hopscotch_map ALIAS hopscotch_map)

add_library(imath INTERFACE)
target_sources(imath INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/imath/imath.c
    ${CMAKE_CURRENT_SOURCE_DIR}/imath/imrat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/imath/gmp_compat.c)
target_include_directories(imath INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imath>")
add_library(imath::imath ALIAS imath)

if (PARSER_BUILD_TESTS)
    set(CATCH_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "" FORCE)
    enable_testing()
    add_subdirectory(Catch2)
endif()

if (NOT PARSER_BUILD_WEB)
    add_subdirectory(CLI11)
endif()
