add_library(gringo_output
    "src/text.cc"
    )

target_link_libraries(gringo_output PUBLIC gringo_core)
target_include_directories(gringo_output PUBLIC "include")
set_property(TARGET gringo_output PROPERTY POSITION_INDEPENDENT_CODE ON)
