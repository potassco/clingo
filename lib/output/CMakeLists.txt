add_library(clingo_output
    "src/text.cc"
    )
add_library(clingo::output ALIAS clingo_output)

target_link_libraries(clingo_output PUBLIC clingo::core)
target_include_directories(clingo_output PUBLIC "include")
set_property(TARGET clingo_output PROPERTY POSITION_INDEPENDENT_CODE ON)
