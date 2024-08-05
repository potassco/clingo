add_library(gringo_util INTERFACE)

target_link_libraries(gringo_util INTERFACE tsl::hopscotch_map tsl::ordered_map)
target_include_directories(gringo_util INTERFACE "include")

if (PARSER_BUILD_TESTS)
    add_executable(test_util
        "tests/record.cc")
    target_link_libraries(test_util PRIVATE gringo_util Catch2::Catch2WithMain)
    target_include_directories(test_util PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    add_test(NAME test_util COMMAND test_util)
endif()
