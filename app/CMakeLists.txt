# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(clingo-bin)
target_sources(clingo-bin PRIVATE ${source})
clingo_target_properties(TARGETS clingo-bin FOLDER exe TYPE default)
set_target_properties(clingo-bin PROPERTIES
    OUTPUT_NAME clingo
)
target_include_directories(clingo-bin PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(clingo-bin PRIVATE clingo clingo::python)
