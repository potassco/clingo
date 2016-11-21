set(source_path ${CLINGO_SOURCE_DIR}/libpyclingo)
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/pyclingo.cc"
    "${source_path}/pyclingo.h")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (CLINGO_BUILD_PY_SHARED)
    set(pyclingo_lib_type SHARED)
else()
    set(pyclingo_lib_type STATIC)
endif()

add_library(libpyclingo ${pyclingo_lib_type} ${source})
target_compile_definitions(libpyclingo PRIVATE PYCLINGO_BUILD_LIBRARY)
set_property(TARGET libpyclingo PROPERTY INTERFACE_COMPILE_DEFINITIONS "CLINGO_WITH_PYTHON")
target_link_libraries(libpyclingo PUBLIC libclingo)
target_link_libraries(libpyclingo PRIVATE Python::Python)
target_include_directories(libpyclingo PUBLIC
    "$<BUILD_INTERFACE:${source_path}>")
set_target_properties(libpyclingo PROPERTIES
    OUTPUT_NAME pyclingo
    FOLDER lib)
if (NOT CLINGO_BUILD_PY_SHARED)
    target_compile_definitions(libpyclingo PUBLIC PYCLINGO_NO_VISIBILITY)
else()
    set_target_properties(libpyclingo PROPERTIES SOVERSION 1 VERSION 1.0)
    install(TARGETS libpyclingo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
endif()
