add_executable(tests "input/parse.cc" "input/variables.cc" "input/rewrite_anonymous.cc" "input/unpool.cc" "input/project.cc" "input/project_anonymous.cc")
target_link_libraries(tests PRIVATE parsertest foonathan::lexy::core Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tests PROPERTIES FOLDER test)
add_test(NAME tests COMMAND tests)
