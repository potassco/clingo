add_library(gringo_ground INTERFACE)

target_link_libraries(gringo_ground INTERFACE gringo_core)
target_include_directories(gringo_ground INTERFACE "include")
