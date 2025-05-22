add_library(clingo_util INTERFACE)
add_library(clingo::util ALIAS clingo_util)
clingo_target_properties(clingo_util lib/util)
target_include_directories(clingo_util
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo_util INTERFACE tsl::hopscotch_map tsl::ordered_map)
clingo_install_target(clingo_util extra)

if (CLINGO_BUILD_TESTS)
    add_executable(test_util
        "tests/record.cc"
        "tests/interval_set.cc")
    clingo_target_properties(test_util test/util tests tests)
    target_include_directories(test_util PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_util PRIVATE clingo::util Catch2::Catch2WithMain)
    catch_discover_tests(test_util)
endif()
