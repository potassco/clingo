set(source_path ${CLINGO_SOURCE_DIR}/app/gringo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(gringo ${header} ${source})
target_link_libraries(gringo PRIVATE libgringo libpotassco)
set_target_properties(gringo PROPERTIES FOLDER exe)

if (CLINGO_ENABLE_LUA)
    target_compile_definitions(gringo PRIVATE WITH_LUA)
    target_include_directories(gringo PRIVATE ${LUA_INCLUDE_DIR})
endif()
if (CLINGO_ENABLE_PYTHON)
    target_compile_definitions(gringo PRIVATE WITH_PYTHON)
    target_include_directories(gringo PRIVATE ${PYTHON_INCLUDE_DIRS})
endif()
