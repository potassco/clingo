add_library(parsertest "src/main.cc" "src/symbol.cc" "src/term.cc" "src/literal.cc" "src/aggregate.cc" "src/theory.cc" "src/head_literal.cc" "src/body_literal.cc" "src/statement.cc")
target_link_libraries(parsertest foonathan::lexy::core foonathan::lexy::file)
target_include_directories(parsertest PUBLIC "include")

add_subdirectory(tests)
