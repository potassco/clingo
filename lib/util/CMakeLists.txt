add_library(clingo_util INTERFACE)
add_library(clingo::util ALIAS clingo_util)

target_link_libraries(clingo_util INTERFACE tsl::hopscotch_map tsl::ordered_map)
target_include_directories(clingo_util INTERFACE "include")

if (PARSER_BUILD_TESTS)
    add_executable(test_util
        "tests/record.cc"
        "tests/interval_set.cc")
    target_link_libraries(test_util PRIVATE clingo::util Catch2::Catch2WithMain)
    target_include_directories(test_util PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    add_test(NAME test_util COMMAND test_util)
    catch_discover_tests(test_util)
endif()
