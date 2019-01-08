# [[[header: .
set(ide_header_group "Header Files")
set(header-group
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo.hh")
source_group("${ide_header_group}" FILES ${header-group})
set(header-group-clingo
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/ast.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/clingo_app.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/clingocontrol.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/control.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/incmode.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/script.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingo/scripts.hh")
source_group("${ide_header_group}\\clingo" FILES ${header-group-clingo})
set(header
    ${header-group}
    ${header-group-clingo})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ast.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingo_app.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingocontrol.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/control.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gringo_app.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/incmode.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/scripts.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

list(APPEND header "${CLASP_SOURCE_DIR}/app/clasp_app.h")
list(APPEND source "${CLASP_SOURCE_DIR}/app/clasp_app.cpp")

if (CLINGO_BUILD_SHARED)
    set(clingo_lib_type SHARED)
else()
    set(clingo_lib_type STATIC)
endif()

add_library(libclingo ${clingo_lib_type} ${header} ${source})
target_link_libraries(libclingo PRIVATE libgringo libclasp)
target_include_directories(libclingo
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
    "$<BUILD_INTERFACE:${CLASP_SOURCE_DIR}/app>")
target_compile_definitions(libclingo PRIVATE CLINGO_BUILD_LIBRARY)
if (NOT CLINGO_BUILD_SHARED)
    target_compile_definitions(libclingo PUBLIC CLINGO_NO_VISIBILITY)
else()
    set_target_properties(libclingo PROPERTIES SOVERSION 3 VERSION 3.0)
    install(TARGETS libclingo
        EXPORT ClingoTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/clingo.hh"
        "${CMAKE_CURRENT_SOURCE_DIR}/clingo.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT ClingoTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Clingo)

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/ClingoConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ClingoConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Clingo)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ClingoConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ClingoConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ClingoConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Clingo)
endif()
if (MSVC)
    target_compile_definitions(libclingo PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()
if (CLINGO_BUILD_REVISION)
    target_compile_definitions(libclingo PRIVATE CLINGO_BUILD_REVISION=${CLINGO_BUILD_REVISION})
endif()
set_target_properties(libclingo PROPERTIES
    OUTPUT_NAME clingo
    FOLDER lib)

if (CLINGO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
