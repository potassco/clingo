set(CMAKE_EXE_LINKER_FLAGS "-s ENVIRONMENT=worker -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")

add_executable(clingo-web "main.cc")
clingo_target_properties(clingo-web web/clingo web)
set_target_properties(clingo-web PROPERTIES
    OUTPUT_NAME clingo
)
target_compile_definitions(clingo-web PRIVATE CLINGO_BUILD_WEB)
target_link_libraries(clingo-web PRIVATE clingo embind)
target_include_directories(clingo-web PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
