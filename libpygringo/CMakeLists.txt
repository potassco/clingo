set(source_path ${CLINGO_SOURCE_DIR}/libpygringo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/python.hh"
    "${source_path}/python.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_library(libpygringo ${source})
target_link_libraries(libpygringo PRIVATE libgringo ${clingo_pygringo_libs})
target_include_directories(libpygringo PUBLIC
    "$<BUILD_INTERFACE:${source_path}>")
set_target_properties(libpygringo PROPERTIES
    OUTPUT_NAME pygringo
    FOLDER lib)
