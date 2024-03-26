add_library(gringo_ground
    "src/instantiator.cc"
    "src/literal.cc"
    "src/program.cc"
    "src/statement.cc"
    "src/term.cc"
    )

target_link_libraries(gringo_ground PUBLIC gringo_core)
target_include_directories(gringo_ground PUBLIC "include")
set_property(TARGET gringo_ground PROPERTY POSITION_INDEPENDENT_CODE ON)
