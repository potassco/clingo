set(source_path ${CLINGO_SOURCE_DIR}/app/clingo)
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/src/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(clingo ${header} ${source})
target_link_libraries(clingo libclingo)
set_target_properties(clingo PROPERTIES FOLDER exe)

install(TARGETS clingo RUNTIME DESTINATION bin)

if (CLINGO_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
