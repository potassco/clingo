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
target_link_libraries(luaclingo PRIVATE libclingo libpotassco)
set_target_properties(luaclingo PROPERTIES
    FOLDER lib
    OUTPUT_NAME clingo
    PREFIX "")
target_include_directories(luaclingo PRIVATE ${LUA_INCLUDE_DIR})
if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(luaclingo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lua)
endif()
if (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(luaclingo PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lua)
endif()
if (CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set_target_properties(luaclingo PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/lua)
endif()
