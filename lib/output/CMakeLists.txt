add_library(clingo_output STATIC
    "src/text.cc"
    "src/backend.cc"
    )
add_library(clingo::output ALIAS clingo_output)
clingo_target_properties(clingo_output lib/output)
target_include_directories(clingo_output PUBLIC "include")
target_link_libraries(clingo_output PUBLIC clingo::core)
