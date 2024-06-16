add_library(gringo_util INTERFACE)

target_link_libraries(gringo_util INTERFACE tsl::hopscotch_map tsl::ordered_map)
target_include_directories(gringo_util INTERFACE "include")
