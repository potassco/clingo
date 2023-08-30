add_library(parsertest
    "src/input/algo/add_sign.cc"
    "src/input/algo/check_type.cc"
    "src/input/algo/evaluate.cc"
    "src/input/algo/parse.cc"
    "src/input/algo/print.cc"
    "src/input/algo/project_anonymous.cc"
    "src/input/algo/project.cc"
    "src/input/algo/rewrite_anonymous.cc"
    "src/input/algo/rewrite_arthithmetic.cc"
    "src/input/algo/rewrite.cc"
    "src/input/algo/unpool.cc"
    "src/input/algo/visit_variables.cc"
    "src/input/literal.cc"
    "src/input/term.cc"
    "src/symbol.cc"
    )

target_link_libraries(parsertest PUBLIC hopscotch_map PRIVATE foonathan::lexy::core foonathan::lexy::file)
target_include_directories(parsertest PUBLIC "include")
# for lexy using >> and + in combination
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(parsertest PUBLIC "-Wno-parentheses")
endif()

if (PARSER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
