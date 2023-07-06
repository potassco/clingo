add_library(parsertest "src/main.cc" "src/symbol.cc" "src/term.cc" "src/literal.cc" "src/aggregate.cc" "src/theory.cc" "src/head_literal.cc" "src/body_literal.cc" "src/statement.cc")
target_link_libraries(parsertest foonathan::lexy::core foonathan::lexy::file)
target_include_directories(parsertest PUBLIC "include")
# for lexy using >> and + in combination
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(parsertest PUBLIC "-Wno-parentheses")
endif()

if (NOT PARSER_BUILD_WEB)
    add_subdirectory(tests)
endif()
