re2c_target_or_gen("src/parse/lexer_impl.xch")

add_library(clingo_input STATIC
    ${RE2C_lexer_impl_OUTPUT}
    "src/rewrite/add_sign.cc"
    "src/rewrite/analyze.cc"
    "src/rewrite/check_syntax.cc"
    "src/rewrite/compute_bounds.cc"
    "src/rewrite/dependency.cc"
    "src/rewrite/evaluate.cc"
    "src/rewrite/iesolver.cc"
    "src/rewrite/project_anonymous.cc"
    "src/rewrite/project.cc"
    "src/rewrite/rewrite_anonymous.cc"
    "src/rewrite/rewrite_context.cc"
    "src/rewrite/rewrite_theory.cc"
    "src/rewrite/safety.cc"
    "src/rewrite/simplify.cc"
    "src/rewrite/substitute.cc"
    "src/rewrite/unpool.cc"
    "src/rewrite/unpool_relations.cc"
    "src/rewrite/visit_variables.cc"
    "src/rewrite.cc"
    "src/parse/body_literal.cc"
    "src/parse/head_literal.cc"
    "src/parse/literal.cc"
    "src/parse/parser_state.cc"
    "src/parse/statement.cc"
    "src/parse/term.cc"
    "src/parse/theory.cc"
    "src/parse/aspif.cc"
    "src/parser.cc"
    "src/print.cc"
    "src/program.cc"
    )
add_library(clingo::input ALIAS clingo_input)
clingo_target_properties(clingo_input lib/input)
target_include_directories(clingo_input
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>")
target_link_libraries(clingo_input PUBLIC clingo::core)

if (PARSER_BUILD_TESTS)
    add_executable(test_input
        "tests/compute_bounds.cc"
        "tests/dependency.cc"
        "tests/evaluate.cc"
        "tests/iesolver.cc"
        "tests/parser.cc"
        "tests/program.cc"
        "tests/project_anonymous.cc"
        "tests/project.cc"
        "tests/rewrite_anonymous.cc"
        "tests/safety.cc"
        "tests/simplify.cc"
        "tests/theory.cc"
        "tests/unpool.cc"
        "tests/unpool_relations.cc"
        "tests/variables.cc")
    clingo_target_properties(test_input test/input tests)
    target_include_directories(test_input PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_input PRIVATE clingo::input Catch2::Catch2WithMain)
    catch_discover_tests(test_input)
endif()
