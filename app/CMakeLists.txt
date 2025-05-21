add_executable(clingo-bin "main.cc")
clingo_target_properties(clingo-bin bin/clingo)
set_target_properties(clingo-bin PROPERTIES
    OUTPUT_NAME clingo
)
target_include_directories(clingo-bin PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(clingo-bin PRIVATE clingo clingo::python)
