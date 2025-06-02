# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo-ground
    "include/clingo/ground/assignment_aggregate.hh"
    "include/clingo/ground/base.hh"
    "include/clingo/ground/body_aggregate.hh"
    "include/clingo/ground/condlit.hh"
    "include/clingo/ground/disjunction.hh"
    "include/clingo/ground/head_aggregate.hh"
    "include/clingo/ground/instantiator.hh"
    "include/clingo/ground/literal.hh"
    "include/clingo/ground/matcher.hh"
    "include/clingo/ground/program.hh"
    "include/clingo/ground/script.hh"
    "include/clingo/ground/statement.hh"
    "include/clingo/ground/term.hh"
    "include/clingo/ground/theory_atom.hh"
    "include/clingo/ground/theory_term.hh")
source_group("${ide_header_group}\\clingo\\ground" FILES ${header-group-clingo-ground})
set(header
    ${header-group-clingo-ground})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "src/assignment_aggregate.cc"
    "src/base.cc"
    "src/body_aggregate.cc"
    "src/condlit.cc"
    "src/disjunction.cc"
    "src/head_aggregate.cc"
    "src/instantiator.cc"
    "src/literal.cc"
    "src/matcher.cc"
    "src/program.cc"
    "src/statement.cc"
    "src/term.cc"
    "src/theory_atom.cc"
    "src/theory_term.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group
    "tests/matcher.cc")
source_group("${ide_test_group}" FILES ${test-group})
set(test
    ${test-group})
# ]]]

add_library(clingo-ground STATIC)
target_sources(clingo-ground PRIVATE ${header} ${source})
clingo_target_properties(TARGETS clingo-ground FOLDER lib TYPE extra)
target_include_directories(clingo-ground
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo-ground PUBLIC clingo-core)
if (CLINGO_PROFILE)
    target_compile_definitions(clingo-ground PUBLIC "CLINGO_PROFILE")
    target_link_libraries(clingo-ground PUBLIC Gperftools::Profiler)
endif()

if (CLINGO_BUILD_TESTS)
    add_executable(test_clingo-ground)
    target_sources(test_clingo-ground PRIVATE ${test})
    clingo_target_properties(TARGETS test_clingo-ground FOLDER test SUBDIR tests)
    target_include_directories(test_clingo-ground PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_clingo-ground PRIVATE clingo-ground Catch2::Catch2WithMain)
    catch_discover_tests(test_clingo-ground DL_PATH "$<TARGET_FILE_DIR:clingo>")
endif()
