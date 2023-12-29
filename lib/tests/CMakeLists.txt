add_executable(tests
    "input/compute_bounds.cc"
    "input/evaluate.cc"
    "input/iesolver.cc"
    "input/parse.cc"
    "input/program.cc"
    "input/project_anonymous.cc"
    "input/project.cc"
    "input/rewrite_anonymous.cc"
    "input/simplify.cc"
    "input/unpool.cc"
    "input/unpool_relations.cc"
    "input/variables.cc"
    "number.cc"
    "symbol.cc")
target_link_libraries(tests PRIVATE parsertest foonathan::lexy::core Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tests PROPERTIES FOLDER test)
add_test(NAME tests COMMAND tests)
