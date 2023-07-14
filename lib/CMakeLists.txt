add_library(parsertest "src/symbol.cc" "src/input/term.cc" "src/input/literal.cc" "src/input/aggregate.cc" "src/input/theory.cc" "src/input/head_literal.cc" "src/input/body_literal.cc" "src/input/statement.cc" "src/input/print.cc")
target_link_libraries(parsertest foonathan::lexy::core foonathan::lexy::file)
target_include_directories(parsertest PUBLIC "include")
# for lexy using >> and + in combination
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(parsertest PUBLIC "-Wno-parentheses")
endif()

if (NOT PARSER_BUILD_WEB)
    add_subdirectory(tests)
endif()
