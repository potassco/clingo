# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(clingo ${header} ${source})
target_link_libraries(clingo PRIVATE libclingo libpyclingo Pyclingo::Pyclingo libluaclingo)
set_target_properties(clingo PROPERTIES FOLDER exe)

install(TARGETS clingo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (CLINGO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
