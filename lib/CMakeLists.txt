add_library(parsertest "src/main.cc")
target_link_libraries(parsertest PRIVATE pegtl)
target_include_directories(parsertest PUBLIC "include")

