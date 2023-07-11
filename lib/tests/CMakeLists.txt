add_executable(tests "input/term.cc" "input/literal.cc" "input/head_literal.cc" "input/body_literal.cc" "input/statement.cc" "input/program.cc" "input/theory.cc" "input/parser.cc")
target_link_libraries(tests PRIVATE parsertest foonathan::lexy::core Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tests PROPERTIES FOLDER test)
add_test(NAME tests COMMAND tests)
