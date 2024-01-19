add_library(gringo
    "src/input/aggregate.cc"
    "src/input/algo/add_sign.cc"
    "src/input/algo/analyze.cc"
    "src/input/algo/check_syntax.cc"
    "src/input/algo/compute_bounds.cc"
    "src/input/algo/evaluate.cc"
    "src/input/algo/parse.cc"
    "src/input/algo/print.cc"
    "src/input/algo/project_anonymous.cc"
    "src/input/algo/project.cc"
    "src/input/algo/rewrite_anonymous.cc"
    "src/input/algo/rewrite.cc"
    "src/input/algo/safety.cc"
    "src/input/algo/simplify.cc"
    "src/input/algo/substitute.cc"
    "src/input/algo/unpool.cc"
    "src/input/algo/unpool_relations.cc"
    "src/input/algo/visit_variables.cc"
    "src/input/body_literal.cc"
    "src/input/iesolver.cc"
    "src/input/literal.cc"
    "src/input/program.cc"
    "src/input/term.cc"
    "src/input/theory.cc"
    "src/number.cc"
    "src/symbol.cc"
    )

target_link_libraries(gringo PUBLIC tcb::span gsl::gsl tsl::hopscotch_map tsl::ordered_map PRIVATE foonathan::lexy::core foonathan::lexy::file imath::imath)
target_include_directories(gringo PUBLIC "include")
set_property(TARGET gringo lexy_core lexy_file PROPERTY POSITION_INDEPENDENT_CODE ON)
# for lexy using >> and + in combination
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(gringo PUBLIC "-Wno-parentheses")
endif()

if (PARSER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
