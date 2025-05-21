add_library(clingo_control STATIC
    "src/grounder.cc"
    "src/solver.cc"
    "src/theory.cc"
    "src/condlit.cc"
    "src/aggregate.cc"
    "src/statement.cc"
    "src/term.cc")
add_library(clingo::control ALIAS clingo_control)
clingo_target_properties(clingo_control lib/control)
target_include_directories(clingo_control PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(clingo_control PUBLIC clingo::input clingo::ground clingo::output libclasp)
clingo_install_target(clingo_control extra)

if(PARSER_BUILD_TESTS)
    add_executable(test_control
        "tests/logger.cc"
        "tests/text.cc")
    clingo_target_properties(test_control test/control tests)
    target_include_directories(test_control PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_control PRIVATE clingo::control Catch2::Catch2WithMain)
    catch_discover_tests(test_control)
endif()
