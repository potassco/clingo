set(source_path ${CLINGO_SOURCE_DIR}/app/pyclingo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/src/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (UNIX)
    set(suffix ".so")
else()
    set(suffix ".pyd")
endif()

add_library(pyclingo SHARED ${source})
target_link_libraries(pyclingo PRIVATE libclingo libpotassco ${PYTHON_LIBRARIES})
set_target_properties(pyclingo PROPERTIES
    FOLDER lib
    OUTPUT_NAME clingo
    SUFFIX ${suffix}
    PREFIX "")
target_include_directories(pyclingo PRIVATE ${PYTHON_INCLUDE_DIRS})
if (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python)
endif()
