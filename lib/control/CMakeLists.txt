add_library(clingo_control
    "src/grounder.cc"
    "src/solver.cc"
    "src/theory.cc"
    "src/condlit.cc"
    "src/aggregate.cc"
    "src/statement.cc"
    "src/term.cc")
add_library(clingo::control ALIAS clingo_control)

target_link_libraries(clingo_control PUBLIC clingo::input clingo::ground clingo::output libclasp)
target_include_directories(clingo_control PUBLIC "include")
set_property(TARGET clingo_control PROPERTY POSITION_INDEPENDENT_CODE ON)

if(PARSER_BUILD_TESTS)
    add_executable(test_control
        "tests/logger.cc"
        "tests/text.cc")
    target_link_libraries(test_control PRIVATE clingo::control Catch2::Catch2WithMain)
    target_include_directories(test_control PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_control PROPERTIES FOLDER test_control)
    if (PARSER_BUILD_WEB)
        add_test(NAME test_control COMMAND test_control)
    else()
        catch_discover_tests(test_control)
    endif()
endif()
