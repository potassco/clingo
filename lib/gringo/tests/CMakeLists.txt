add_executable(tests
    "gringo/input/compute_bounds.cc"
    "gringo/input/evaluate.cc"
    "gringo/input/iesolver.cc"
    "gringo/input/parse.cc"
    "gringo/input/program.cc"
    "gringo/input/project_anonymous.cc"
    "gringo/input/project.cc"
    "gringo/input/rewrite_anonymous.cc"
    "gringo/input/safety.cc"
    "gringo/input/simplify.cc"
    "gringo/input/unpool.cc"
    "gringo/input/unpool_relations.cc"
    "gringo/input/variables.cc"
    "gringo/number.cc"
    "gringo/symbol.cc")
target_link_libraries(tests PRIVATE gringo foonathan::lexy::core Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tests PROPERTIES FOLDER test)
add_test(NAME tests COMMAND tests)
