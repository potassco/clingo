# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

set(CMAKE_EXE_LINKER_FLAGS "-s ENVIRONMENT=worker -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")

add_executable(clingo-web)
target_sources(clingo-web PRIVATE ${source})
clingo_target_properties(clingo-web web web)
set_target_properties(clingo-web PROPERTIES
    OUTPUT_NAME clingo
)
target_compile_definitions(clingo-web PRIVATE CLINGO_BUILD_WEB)
target_link_libraries(clingo-web PRIVATE clingo embind)
target_include_directories(clingo-web PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
