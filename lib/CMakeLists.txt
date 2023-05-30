add_library(parsertest "src/main.cc" "src/term.cc")
target_link_libraries(parsertest foonathan::lexy::core foonathan::lexy::file)
target_include_directories(parsertest PUBLIC "include")

add_subdirectory(tests)
