add_executable(tests "term.cc" "literal.cc" "head_literal.cc" "body_literal.cc" "statement.cc" "program.cc" "simple.cc" "theory.cc" "parser.cc")
target_link_libraries(tests PRIVATE parsertest foonathan::lexy::core Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tests PROPERTIES FOLDER test)
add_test(NAME tests COMMAND tests)

