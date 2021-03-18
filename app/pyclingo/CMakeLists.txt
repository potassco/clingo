# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/_clingo.c")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (PYCLINGO_INSTALL_DIR STREQUAL "" AND Python_EXECUTABLE)
    if (PYCLINGO_INSTALL STREQUAL "user")
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py site OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    elseif(PYCLINGO_INSTALL STREQUAL "prefix")
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py prefix "${CMAKE_INSTALL_PREFIX}" OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py prefix OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()

if (PYCLINGO_SUFFIX STREQUAL "")
    if (Python_EXECUTABLE)
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py suffix OUTPUT_VARIABLE PYCLINGO_SUFFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        if (CYGWIN)
            set(PYCLINGO_SUFFIX ".dll")
        elseif (UNIX)
            set(PYCLINGO_SUFFIX ".so")
        else()
            set(PYCLINGO_SUFFIX ".pyd")
        endif()
    endif()
endif()

add_library(pyclingo MODULE ${source})
target_link_libraries(pyclingo PRIVATE libclingo Python::Module)
# NOTE: It would be really nice to enable the limited API for the pip packages
# to avoid having to build Python-specific wheels. Unfortunately, this fails to
# build on conda on Windows then. Since supporting conda is important, it is
# disabled for now but should be investigated further, later.
target_compile_definitions(pyclingo PRIVATE _CFFI_NO_LIMITED_API)
# NOTE: This seems to be necessary on MacOS. I do not understand why.
if (PYCLINGO_DYNAMIC_LOOKUP)
    set_property(TARGET pyclingo PROPERTY LINK_FLAGS "-undefined dynamic_lookup")
endif()
set_target_properties(pyclingo PROPERTIES
    FOLDER lib
    OUTPUT_NAME _clingo
    SUFFIX ${PYCLINGO_SUFFIX}
    PREFIX "")
if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python)
endif()
if (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python)
endif()
if (CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/python)
endif()

if (PYCLINGO_INSTALL_DIR)
    file(TO_CMAKE_PATH "${PYCLINGO_INSTALL_DIR}" _PYCLINGO_INSTALL_DIR)
    install(TARGETS pyclingo
        RUNTIME DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo
        LIBRARY DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo
        ARCHIVE DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo)
    if (CLINGO_BUILD_WITH_PYTHON STREQUAL "pip")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/INSTALLER "pip\n")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/METADATA "Metadata-Version: 1.1\nName: clingo\nVersion: ${CLINGO_VERSION}\n")
        install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/INSTALLER
            ${CMAKE_CURRENT_BINARY_DIR}/METADATA
            DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo-${CLINGO_VERSION}.dist-info)
    endif()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/libpyclingo/clingo/
        DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo
        FILES_MATCHING
        PATTERN "*.py"
        PATTERN "*.typed"
        PATTERN "tests" EXCLUDE)
    install(FILES
        ${CMAKE_SOURCE_DIR}/libclingo/clingo.h
        ${CMAKE_SOURCE_DIR}/libclingo/clingo.hh
        DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo)
endif()

if (Python_EXECUTABLE AND CLINGO_BUILD_TESTS)
    add_test(NAME test_pyclingo
             WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/libpyclingo"
             COMMAND ${Python_EXECUTABLE} -m unittest)
    set_property(TEST test_pyclingo APPEND PROPERTY ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pyclingo>")
    set_property(TEST test_pyclingo APPEND PROPERTY ENVIRONMENT "PATH=$<TARGET_FILE_DIR:libclingo>")
endif()
