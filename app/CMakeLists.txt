add_executable(clingo-bin "main.cc")
set_target_properties(clingo-bin PROPERTIES OUTPUT_NAME clingo)
target_link_libraries(clingo-bin PRIVATE clingo clingo::python)
target_include_directories(clingo-bin PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
