add_library(parsertest "src/main.cc")
target_link_libraries(parsertest foonathan::lexy::core)
target_include_directories(parsertest PUBLIC "include")

add_subdirectory(tests)
