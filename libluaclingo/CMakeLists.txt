set(source_path ${CLINGO_SOURCE_DIR}/libluaclingo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/lua.hh"
    "${source_path}/lua.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_library(libluaclingo ${source})
target_compile_definitions(libluaclingo PRIVATE LUACLINGO_BUILD_LIBRARY)
target_link_libraries(libluaclingo PUBLIC libclingo ${clingo_luaclingo_libs})
target_include_directories(libluaclingo PUBLIC
    "$<BUILD_INTERFACE:${source_path}>")
set_target_properties(libluaclingo PROPERTIES
    OUTPUT_NAME luaclingo
    FOLDER lib)
