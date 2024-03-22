add_library(gringo_ground
    "src/term.cc"
    "src/literal.cc"
    )

target_link_libraries(gringo_ground PUBLIC gringo_core)
target_include_directories(gringo_ground PUBLIC "include")
set_property(TARGET gringo_ground PROPERTY POSITION_INDEPENDENT_CODE ON)
