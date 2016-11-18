set(source_path ${CLINGO_SOURCE_DIR}/libpyclingo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/python.hh"
    "${source_path}/python.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_library(libpyclingo ${source})
target_compile_definitions(libpyclingo PRIVATE PYCLINGO_BUILD_LIBRARY)
target_link_libraries(libpyclingo PUBLIC libclingo ${clingo_pyclingo_libs})
target_include_directories(libpyclingo PUBLIC
    "$<BUILD_INTERFACE:${source_path}>")
set_target_properties(libpyclingo PROPERTIES
    OUTPUT_NAME pyclingo
    FOLDER lib)
