set(source_path ${CLINGO_SOURCE_DIR}/app/reify)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(reify ${header} ${source})
target_link_libraries(reify PRIVATE libreify libpotassco)
set_target_properties(reify PROPERTIES FOLDER exe)

install(TARGETS reify RUNTIME DESTINATION bin)
