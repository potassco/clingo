# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo-control
    "include/clingo/control/aggregate.hh"
    "include/clingo/control/condlit.hh"
    "include/clingo/control/context.hh"
    "include/clingo/control/grounder.hh"
    "include/clingo/control/literal.hh"
    "include/clingo/control/parse.hh"
    "include/clingo/control/solver.hh"
    "include/clingo/control/statement.hh"
    "include/clingo/control/term.hh"
    "include/clingo/control/theory.hh")
source_group("${ide_header_group}\\clingo\\control" FILES ${header-group-clingo-control})
set(header
    ${header-group-clingo-control})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "src/aggregate.cc"
    "src/condlit.cc"
    "src/grounder.cc"
    "src/solver.cc"
    "src/statement.cc"
    "src/term.cc"
    "src/theory.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group
    "tests/logger.cc"
    "tests/text.cc")
source_group("${ide_test_group}" FILES ${test-group})
set(test
    ${test-group})
# ]]]

add_library(clingo_control STATIC)
target_sources(clingo_control PRIVATE ${header} ${source})
add_library(clingo::control ALIAS clingo_control)
clingo_target_properties(clingo_control lib)
target_include_directories(clingo_control PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(clingo_control PUBLIC clingo::input clingo::ground clingo::output libclasp)
clingo_install_target(clingo_control extra)

if(CLINGO_BUILD_TESTS)
    add_executable(test_control)
    target_sources(test_control PRIVATE ${test})
    clingo_target_properties(test_control test tests)
    target_include_directories(test_control PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_control PRIVATE clingo::control Catch2::Catch2WithMain)
    catch_discover_tests(test_control)
endif()
