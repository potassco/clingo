# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo-output
    "include/clingo/output/backend.hh"
    "include/clingo/output/text.hh")
source_group("${ide_header_group}\\clingo\\output" FILES ${header-group-clingo-output})
set(header
    ${header-group-clingo-output})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "src/analyzer.hh"
    "src/backend.cc"
    "src/text.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_library(clingo_output STATIC)
target_sources(clingo_output PRIVATE ${header} ${source})
add_library(clingo::output ALIAS clingo_output)
clingo_target_properties(TARGETS clingo_output FOLDER lib TYPE extra)
target_include_directories(clingo_output
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo_output PUBLIC clingo::core)
