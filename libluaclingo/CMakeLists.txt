set(source_path ${CLINGO_SOURCE_DIR}/libluaclingo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/lua.cc"
    "${source_path}/lua.hh")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (CLINGO_BUILD_LUA_SHARED)
    set(luaclingo_lib_type SHARED)
else()
    set(luaclingo_lib_type STATIC)
endif()

add_library(libluaclingo ${luaclingo_lib_type} ${source})
target_compile_definitions(libluaclingo PRIVATE LUACLINGO_BUILD_LIBRARY)
set_property(TARGET libluaclingo PROPERTY INTERFACE_COMPILE_DEFINITIONS "CLINGO_WITH_LUA")
target_link_libraries(libluaclingo PUBLIC libclingo)
target_link_libraries(libluaclingo PRIVATE Lua::Lua)
target_include_directories(libluaclingo PUBLIC
    "$<BUILD_INTERFACE:${source_path}>")
set_target_properties(libluaclingo PROPERTIES
    OUTPUT_NAME luaclingo
    FOLDER lib)
if (NOT CLINGO_BUILD_LUA_SHARED)
    target_compile_definitions(libluaclingo PUBLIC LUACLINGO_NO_VISIBILITY)
else()
    set_target_properties(libluaclingo PROPERTIES SOVERSION 1 VERSION 1.0)
    install(TARGETS libluaclingo
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
endif()
