# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (NOT PYCLINGO_INSTALL_DIR AND PYTHON_EXECUTABLE)
    if (PYCLINGO_USER_INSTALL)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -m site --user-site OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    elseif(PYCLINGO_USE_INSTALL_PREFIX)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py prefix "${CMAKE_INSTALL_PREFIX}" OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py prefix OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()

if (NOT PYCLINGO_SUFFIX)
    if (PYTHON_EXECUTABLE)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py suffix OUTPUT_VARIABLE PYCLINGO_SUFFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
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

add_library(pyclingo SHARED ${source})
target_link_libraries(pyclingo PRIVATE libpyclingo)
set_target_properties(pyclingo PROPERTIES
    FOLDER lib
    OUTPUT_NAME clingo
    SUFFIX ${PYCLINGO_SUFFIX}
    PREFIX "")

if (PYCLINGO_INSTALL_DIR)
    install(TARGETS pyclingo
        RUNTIME DESTINATION ${PYCLINGO_INSTALL_DIR}
        LIBRARY DESTINATION ${PYCLINGO_INSTALL_DIR}
        ARCHIVE DESTINATION ${PYCLINGO_INSTALL_DIR})
endif()

target_include_directories(pyclingo PRIVATE ${PYTHON_INCLUDE_DIRS})
if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python)
endif()
if (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python)
endif()
if (CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set_target_properties(pyclingo PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/python)
endif()
