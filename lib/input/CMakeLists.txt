re2c_target_or_gen("src/algo/parse/lexer_impl.xch")

add_library(gringo_input
    ${RE2C_lexer_impl_OUTPUT}
    "src/algo/add_sign.cc"
    "src/algo/analyze.cc"
    "src/algo/check_syntax.cc"
    "src/algo/compute_bounds.cc"
    "src/algo/dependency.cc"
    "src/algo/evaluate.cc"
    "src/algo/iesolver.cc"
    "src/algo/parse.cc"
    "src/algo/parse/parser_state.cc"
    "src/algo/parse/parsev2.cc"
    "src/algo/parse/termv2.cc"
    "src/algo/print.cc"
    "src/algo/project_anonymous.cc"
    "src/algo/project.cc"
    "src/algo/rewrite_anonymous.cc"
    "src/algo/rewrite.cc"
    "src/algo/rewrite_context.cc"
    "src/algo/rewrite_theory.cc"
    "src/algo/safety.cc"
    "src/algo/simplify.cc"
    "src/algo/substitute.cc"
    "src/algo/unpool.cc"
    "src/algo/unpool_relations.cc"
    "src/algo/visit_variables.cc"
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
        "tests/parse.cc"
        "tests/parsev2.cc"
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
