# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/pyclingo.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/pyclingo.h")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (CLINGO_BUILD_PY_SHARED)
    set(pyclingo_lib_type SHARED)
else()
    set(pyclingo_lib_type STATIC)
endif()

# for embedding
add_library(libpyclingo ${pyclingo_lib_type} ${source})
target_link_libraries(libpyclingo PUBLIC libclingo PRIVATE Python::Python)
target_include_directories(libpyclingo PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_definitions(libpyclingo PRIVATE PYCLINGO_BUILD_LIBRARY PUBLIC CLINGO_WITH_PYTHON)
if (NOT CLINGO_BUILD_PY_SHARED)
    target_compile_definitions(libpyclingo PUBLIC PYCLINGO_NO_VISIBILITY)
else()
    set_target_properties(libpyclingo PROPERTIES SOVERSION 1 VERSION 1.0)
    install(TARGETS libpyclingo
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
set_target_properties(libpyclingo PROPERTIES
    OUTPUT_NAME pyclingo
    FOLDER lib)

# for module
add_library(libpyclingom STATIC ${source})
target_link_libraries(libpyclingom PUBLIC libclingo Python::Module)
target_include_directories(libpyclingom PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_definitions(libpyclingom PRIVATE PYCLINGO_BUILD_LIBRARY PUBLIC CLINGO_WITH_PYTHON)
target_compile_definitions(libpyclingom PUBLIC PYCLINGO_NO_VISIBILITY)
set_target_properties(libpyclingom PROPERTIES
    OUTPUT_NAME pyclingom
    FOLDER lib)
