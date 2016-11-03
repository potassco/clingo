set(source_path ${CLINGO_SOURCE_DIR}/app/luaclingo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/src/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_library(luaclingo SHARED ${source})
target_link_libraries(luaclingo libclingo libpotassco)
set_target_properties(luaclingo PROPERTIES
    FOLDER lib
    OUTPUT_NAME clingo
    PREFIX "")
target_include_directories(luaclingo PRIVATE ${LUA_INCLUDE_DIR})
