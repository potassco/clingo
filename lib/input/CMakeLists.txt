re2c_target_or_gen("src/parse/lexer_impl.xch")

add_library(gringo_input
    ${RE2C_lexer_impl_OUTPUT}
    "src/rewrite/add_sign.cc"
    "src/rewrite/analyze.cc"
    "src/rewrite/check_syntax.cc"
    "src/rewrite/compute_bounds.cc"
    "src/rewrite/dependency.cc"
    "src/rewrite/evaluate.cc"
    "src/rewrite/iesolver.cc"
    "src/rewrite/parse.cc"
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
    "src/parse/params.cc"
    "src/parse/statement.cc"
    "src/parse/term.cc"
    "src/parse/theory.cc"
    "src/parser.cc"
    "src/print.cc"
    "src/program.cc"
    )

target_link_libraries(gringo_input PUBLIC gringo_core PRIVATE foonathan::lexy::core foonathan::lexy::file)
target_include_directories(gringo_input PUBLIC "include"
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>")
set_property(TARGET gringo_input lexy_core lexy_file PROPERTY POSITION_INDEPENDENT_CODE ON)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # no-parentheses: for lexy using >> and + in combination
    # no-maybe-uninitialized/no-restrict: for older gccc versions
    target_compile_options(gringo_input PUBLIC "-Wno-parentheses" "-Wno-maybe-uninitialized" "-Wno-restrict" "$<$<COMPILE_LANGUAGE:CXX>:-Wno-non-template-friend>")
endif()

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
    target_link_libraries(test_input PRIVATE gringo_input Catch2::Catch2WithMain)
    target_include_directories(test_input PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    add_test(NAME test_input COMMAND test_input)
endif()
