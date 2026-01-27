# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo-input
    "include/clingo/input/aggregate.hh"
    "include/clingo/input/attributes.hh"
    "include/clingo/input/body_literal.hh"
    "include/clingo/input/head_literal.hh"
    "include/clingo/input/literal.hh"
    "include/clingo/input/parser.hh"
    "include/clingo/input/print.hh"
    "include/clingo/input/program.hh"
    "include/clingo/input/rewrite.hh"
    "include/clingo/input/statement.hh"
    "include/clingo/input/term.hh"
    "include/clingo/input/theory.hh")
source_group("${ide_header_group}\\clingo\\input" FILES ${header-group-clingo-input})
set(header-group-clingo-input-rewrite
    "include/clingo/input/rewrite/analyze.hh"
    "include/clingo/input/rewrite/check_syntax.hh"
    "include/clingo/input/rewrite/compute_bounds.hh"
    "include/clingo/input/rewrite/dependency.hh"
    "include/clingo/input/rewrite/evaluate.hh"
    "include/clingo/input/rewrite/iesolver.hh"
    "include/clingo/input/rewrite/project.hh"
    "include/clingo/input/rewrite/project_anonymous.hh"
    "include/clingo/input/rewrite/rewrite_anonymous.hh"
    "include/clingo/input/rewrite/rewrite_context.hh"
    "include/clingo/input/rewrite/rewrite_theory.hh"
    "include/clingo/input/rewrite/safety.hh"
    "include/clingo/input/rewrite/simplify.hh"
    "include/clingo/input/rewrite/substitute.hh"
    "include/clingo/input/rewrite/unpool.hh"
    "include/clingo/input/rewrite/unpool_relations.hh"
    "include/clingo/input/rewrite/visit_variables.hh")
source_group("${ide_header_group}\\clingo\\input\\rewrite" FILES ${header-group-clingo-input-rewrite})
set(header
    ${header-group-clingo-input}
    ${header-group-clingo-input-rewrite})
# ]]]
# [[[source: src
re2c_target_or_gen("src/parse/lexer_impl.xch")
set(ide_source_group "Source Files")
set(source-group
    "src/parser.cc"
    "src/print.cc"
    "src/program.cc"
    "src/rewrite.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source-group-parse
    "src/parse/aspif.cc"
    "src/parse/body_literal.cc"
    "src/parse/head_literal.cc"
    "src/parse/lexer_impl.xch"
    ${RE2C_lexer_impl_OUTPUT}
    "src/parse/lexer_state.hh"
    "src/parse/literal.cc"
    "src/parse/parser_state.cc"
    "src/parse/parser_state.hh"
    "src/parse/statement.cc"
    "src/parse/term.cc"
    "src/parse/theory.cc")
source_group("${ide_source_group}\\parse" FILES ${source-group-parse})
set(source-group-rewrite
    "src/rewrite/add_sign.cc"
    "src/rewrite/add_sign.hh"
    "src/rewrite/analyze.cc"
    "src/rewrite/check_syntax.cc"
    "src/rewrite/compute_bounds.cc"
    "src/rewrite/dependency.cc"
    "src/rewrite/evaluate.cc"
    "src/rewrite/iesolver.cc"
    "src/rewrite/project.cc"
    "src/rewrite/project_anonymous.cc"
    "src/rewrite/rewrite_anonymous.cc"
    "src/rewrite/rewrite_context.cc"
    "src/rewrite/rewrite_theory.cc"
    "src/rewrite/safety.cc"
    "src/rewrite/simplify.cc"
    "src/rewrite/substitute.cc"
    "src/rewrite/transform.hh"
    "src/rewrite/unpool.cc"
    "src/rewrite/unpool.hh"
    "src/rewrite/unpool_relations.cc"
    "src/rewrite/visit.hh"
    "src/rewrite/visit_variables.cc")
source_group("${ide_source_group}\\rewrite" FILES ${source-group-rewrite})
set(source
    ${source-group}
    ${source-group-parse}
    ${source-group-rewrite})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group
    "tests/aspif.cc"
    "tests/compute_bounds.cc"
    "tests/dependency.cc"
    "tests/evaluate.cc"
    "tests/iesolver.cc"
    "tests/parser.cc"
    "tests/program.cc"
    "tests/project.cc"
    "tests/project_anonymous.cc"
    "tests/rewrite_anonymous.cc"
    "tests/safety.cc"
    "tests/simplify.cc"
    "tests/test.hh"
    "tests/theory.cc"
    "tests/unpool.cc"
    "tests/unpool_relations.cc"
    "tests/variables.cc")
source_group("${ide_test_group}" FILES ${test-group})
set(test
    ${test-group})
# ]]]

add_library(clingo-input STATIC)
target_sources(clingo-input PRIVATE ${header} ${source})
clingo_target_properties(TARGETS clingo-input FOLDER lib TYPE extra)
target_include_directories(clingo-input
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>")
target_link_libraries(clingo-input PUBLIC clingo-core)

if(CLINGO_BUILD_TESTS)
    add_executable(test_clingo-input)
    target_sources(test_clingo-input PRIVATE ${test})
    clingo_target_properties(TARGETS test_clingo-input FOLDER test SUBDIR tests)
    target_include_directories(test_clingo-input PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_clingo-input PRIVATE clingo-input Catch2::Catch2WithMain)
    catch_discover_tests(test_clingo-input DL_PATH "$<TARGET_FILE_DIR:clingo>")
endif()
