add_library(clingo_core
    "src/number.cc"
    "src/symbol.cc"
    "src/core.cc"
    )

target_link_libraries(clingo_core PUBLIC clingo_util tsl::hopscotch_map tsl::ordered_map PRIVATE imath::imath)
target_include_directories(clingo_core PUBLIC "include")
set_property(TARGET clingo_core PROPERTY POSITION_INDEPENDENT_CODE ON)

if (PARSER_BUILD_TESTS)
    add_executable(test_core
        "tests/number.cc"
        "tests/symbol.cc")
    target_link_libraries(test_core PRIVATE clingo_core Catch2::Catch2WithMain)
    target_include_directories(test_core PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_core PROPERTIES FOLDER test_core)
    add_test(NAME test_core COMMAND test_core)
endif()
