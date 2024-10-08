add_library(clingo_output
    "src/text.cc"
    )

target_link_libraries(clingo_output PUBLIC clingo_core)
target_include_directories(clingo_output PUBLIC "include")
set_property(TARGET clingo_output PROPERTY POSITION_INDEPENDENT_CODE ON)
