add_library(parsertest 
    "src/symbol.cc"
    "src/input/term.cc"
    "src/input/literal.cc"
    "src/input/body_literal.cc"
    "src/input/algo/print.cc"
    "src/input/algo/rewrite_anonymous.cc" 
    "src/input/algo/check_type.cc"
    "src/input/algo/project.cc"
    "src/input/algo/rewrite.cc"
    "src/input/algo/unpool.cc"
    "src/input/algo/visit_variables.cc"
    "src/input/algo/parse.cc"
    "src/input/algo/project_anonymous.cc"
    )
target_link_libraries(parsertest PRIVATE foonathan::lexy::core foonathan::lexy::file)
target_include_directories(parsertest PUBLIC "include")
# for lexy using >> and + in combination
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(parsertest PUBLIC "-Wno-parentheses")
endif()

if (NOT PARSER_BUILD_WEB)
    add_subdirectory(tests)
endif()
