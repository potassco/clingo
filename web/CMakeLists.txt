set(CMAKE_EXE_LINKER_FLAGS "-s ENVIRONMENT=worker -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")

add_executable(web "main.cc")
target_compile_definitions(web PRIVATE CLINGO_BUILD_WEB)
target_link_libraries(web PRIVATE clingo embind)
target_include_directories(web PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
set_target_properties(web PROPERTIES OUTPUT_NAME "clingo")
