set(header_path ${CLINGO_SOURCE_DIR}/libreify)
# [[[header: .
set(ide_header_group "Header Files")
set(header-group-reify
    "${header_path}/reify/program.hh"
    "${header_path}/reify/util.hh")
source_group("${ide_header_group}\\reify" FILES ${header-group-reify})
set(header
    ${header-group-reify})
# ]]]
set(source_path ${CLINGO_SOURCE_DIR}/libreify)
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/src/program.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_library(libreify ${header} ${source})
target_include_directories(libreify PUBLIC
    "$<BUILD_INTERFACE:${header_path}>"
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libgringo>")
target_link_libraries(libreify PUBLIC libpotassco)
set_target_properties(libreify PROPERTIES
    OUTPUT_NAME reify
    FOLDER lib)

add_subdirectory(tests)
