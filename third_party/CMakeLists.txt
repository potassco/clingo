add_subdirectory(lexy)

add_library(hopscotch_map INTERFACE)
target_include_directories(hopscotch_map INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hopscotch-map/include>)
add_library(tsl::hopscotch_map ALIAS hopscotch_map)

if (PARSER_BUILD_TESTS)
    set(CATCH_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "" FORCE)
    enable_testing()
    add_subdirectory(Catch2)
endif()

