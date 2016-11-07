set(header_path ${CLINGO_SOURCE_DIR}/libclingo)
# [[[header: .
set(ide_header_group "Header Files")
set(header-group
    "${header_path}/clingo.hh")
source_group("${ide_header_group}" FILES ${header-group})
set(header-group-clingo
    "${header_path}/clingo/clingocontrol.hh"
    "${header_path}/clingo/clingo_app.hh")
source_group("${ide_header_group}\\clingo" FILES ${header-group-clingo})
set(header
    ${header-group}
    ${header-group-clingo})
# ]]]

set(source_path ${CLINGO_SOURCE_DIR}/libclingo)
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/src/clingo_app.cc"
    "${source_path}/src/clingocontrol.cc"
    "${source_path}/src/control.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

list(APPEND header "${CLASP_SOURCE_DIR}/app/clasp_app.h")
list(APPEND source "${CLASP_SOURCE_DIR}/app/clasp_app.cpp")

if (CLINGO_ENABLE_SHARED)
    set(clingo_lib_type SHARED)
else()
    set(clingo_lib_type STATIC)
endif()

add_library(libclingo ${clingo_lib_type} ${clingo_lib_type} ${header} ${source})
target_include_directories(libclingo PRIVATE
    "$<BUILD_INTERFACE:${CLASP_SOURCE_DIR}/app>"
    "$<BUILD_INTERFACE:${header_path}>")
target_include_directories(libclingo PUBLIC
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libgringo>"
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libclingo>")
target_link_libraries(libclingo PRIVATE libgringo libclasp ${clingo_external_libs})
target_compile_definitions(libclingo PRIVATE CLINGO_BUILD_LIBRARY)
set_target_properties(libclingo PROPERTIES
    OUTPUT_NAME clingo
    FOLDER lib)
if (NOT CLINGO_ENABLE_SHARED)
    target_compile_definitions(libclingo PUBLIC CLINGO_NO_VISIBILITY)
endif()

if (CLINGO_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
