add_library(clingo_core STATIC
    "src/core.cc"
    "src/number.cc"
    "src/symbol.cc"
    )
add_library(clingo::core ALIAS clingo_core)
clingo_target_properties(clingo_core lib/core)
target_include_directories(clingo_core
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo_core PUBLIC clingo::util tsl::hopscotch_map tsl::ordered_map PRIVATE imath::imath)

if (PARSER_BUILD_TESTS)
    add_executable(test_core
        "tests/number.cc"
        "tests/symbol.cc")
    clingo_target_properties(test_core test/core tests)
    target_include_directories(test_core PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_core PRIVATE clingo::core Catch2::Catch2WithMain)
    catch_discover_tests(test_core)
endif()
