add_library(parsertest
    "src/gringo/input/algo/add_sign.cc"
    "src/gringo/input/algo/analyze.cc"
    "src/gringo/input/algo/check_syntax.cc"
    "src/gringo/input/algo/compute_bounds.cc"
    "src/gringo/input/algo/evaluate.cc"
    "src/gringo/input/algo/parse.cc"
    "src/gringo/input/algo/print.cc"
    "src/gringo/input/algo/project_anonymous.cc"
    "src/gringo/input/algo/project.cc"
    "src/gringo/input/algo/rewrite_anonymous.cc"
    "src/gringo/input/algo/rewrite.cc"
    "src/gringo/input/algo/safety.cc"
    "src/gringo/input/algo/simplify.cc"
    "src/gringo/input/algo/substitute.cc"
    "src/gringo/input/algo/unpool.cc"
    "src/gringo/input/algo/unpool_relations.cc"
    "src/gringo/input/algo/visit_variables.cc"
    "src/gringo/input/iesolver.cc"
    "src/gringo/input/literal.cc"
    "src/gringo/input/program.cc"
    "src/gringo/input/term.cc"
    "src/gringo/number.cc"
    "src/gringo/symbol.cc"
    )

target_link_libraries(parsertest PUBLIC tcb::span gsl::gsl tsl::hopscotch_map tsl::ordered_map PRIVATE foonathan::lexy::core foonathan::lexy::file imath::imath)
target_include_directories(parsertest PUBLIC "include")
# for lexy using >> and + in combination
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(parsertest PUBLIC "-Wno-parentheses")
endif()

add_library(clingo
    "src/c-api/core.cc"
    "src/c-api/symbol.cc"
    )
target_link_libraries(clingo PRIVATE parsertest)
target_include_directories(clingo PUBLIC "include")

if (PARSER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
