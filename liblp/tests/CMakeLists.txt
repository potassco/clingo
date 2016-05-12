include_directories(${PROJECT_SOURCE_DIR}/liblp/ .)
aux_source_directory(. SRC_LIST)
add_executable(test EXCLUDE_FROM_ALL ${SRC_LIST})
target_link_libraries(test liblp)

