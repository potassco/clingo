add_library(gringo_grounder
    "src/grounder.cc")

target_link_libraries(gringo_grounder PUBLIC gringo_input gringo_ground)
target_include_directories(gringo_grounder PUBLIC "include")
set_property(TARGET gringo_grounder PROPERTY POSITION_INDEPENDENT_CODE ON)
