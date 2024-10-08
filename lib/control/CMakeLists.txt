add_library(clingo_control
    "src/grounder.cc"
    "src/solver.cc"
    "src/theory.cc"
    "src/condlit.cc"
    "src/aggregate.cc"
    "src/statement.cc"
    "src/term.cc")

target_link_libraries(clingo_control PUBLIC clingo_input clingo_ground clingo_output)
target_include_directories(clingo_control PUBLIC "include")
set_property(TARGET clingo_control PROPERTY POSITION_INDEPENDENT_CODE ON)

if (PARSER_BUILD_TESTS)
    add_executable(test_control
        "tests/text.cc")
    target_link_libraries(test_control PRIVATE clingo_control Catch2::Catch2WithMain)
    target_include_directories(test_control PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_control PROPERTIES FOLDER test_control)
    add_test(NAME test_control COMMAND test_control)
endif()
