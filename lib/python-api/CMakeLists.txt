# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "src/app.cc"
    "src/app.hh"
    "src/ast.cc"
    "src/ast.hh"
    "src/backend.cc"
    "src/backend.hh"
    "src/base.cc"
    "src/base.hh"
    "src/clingo.cc"
    "src/clingo.hh"
    "src/config.cc"
    "src/config.hh"
    "src/control.cc"
    "src/control.hh"
    "src/core.cc"
    "src/core.hh"
    "src/embed.cc"
    "src/iterable.hh"
    "src/iterator.hh"
    "src/module.cc"
    "src/propagate.cc"
    "src/propagate.hh"
    "src/script.cc"
    "src/script.hh"
    "src/solve.cc"
    "src/solve.hh"
    "src/stats.cc"
    "src/stats.hh"
    "src/symbol.cc"
    "src/symbol.hh"
    "src/theory.cc"
    "src/theory.hh"
    "src/util.hh")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

list(FILTER source EXCLUDE REGEX "/(embed|module)\\.cc$")

# functionality shared between embedded library and standalone module
add_library(clingo-python STATIC)
target_sources(clingo-python PRIVATE ${source})
clingo_target_properties(TARGETS clingo-python FOLDER lib)
target_compile_definitions(clingo-python PRIVATE CLINGO_PYTHON_VERSION="${Python_VERSION}")
target_link_libraries(clingo-python PRIVATE pybind11::pybind11 clingo)

# the standalone module
pybind11_add_module(pyclingo "src/module.cc")
set_target_properties(pyclingo PROPERTIES IMPORT_LIBRARY "pyclingo.lib")
clingo_target_properties(TARGETS pyclingo FOLDER lib SUBDIR python)
set_target_properties(pyclingo PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(pyclingo PRIVATE clingo clingo-python)
if(CLINGO_USE_VERSION_SCRIPT AND UNIX AND NOT APPLE)
    set_target_properties(pyclingo PROPERTIES LINK_FLAGS "-Wl,--version-script='${CMAKE_SOURCE_DIR}/cmake/pyclingo.map'")
endif()
if(CLINGO_INSTALL_WHEEL)
    install(DIRECTORY stubs/ DESTINATION clingo-stubs)
    install(TARGETS pyclingo
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION ".")
    if(CLINGO_BUILD_SHARED)
        if(APPLE)
            set_target_properties(pyclingo
                PROPERTIES INSTALL_RPATH "@loader_path"
                BUILD_WITH_INSTALL_RPATH TRUE)
        elseif(UNIX)
            set_target_properties(pyclingo
                PROPERTIES INSTALL_RPATH "$ORIGIN"
                BUILD_WITH_INSTALL_RPATH TRUE)
        endif()
    endif()
elseif(CLINGO_INSTALL_DEFAULT)
    if(PYCLINGO_INSTALL_DIR STREQUAL "")
        if(PYCLINGO_INSTALL STREQUAL "user")
            execute_process(
                COMMAND "${Python_EXECUTABLE}" "${CLINGO_SOURCE_DIR}/cmake/python-site.py" target --user
                OUTPUT_VARIABLE _python_install_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        elseif(PYCLINGO_INSTALL STREQUAL "prefix")
            execute_process(
                COMMAND "${Python_EXECUTABLE}" "${CLINGO_SOURCE_DIR}/cmake/python-site.py" target --prefix "${CMAKE_INSTALL_PREFIX}"
                OUTPUT_VARIABLE _python_install_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        else()
            execute_process(
                COMMAND "${Python_EXECUTABLE}" "${CLINGO_SOURCE_DIR}/cmake/python-site.py" target
                OUTPUT_VARIABLE _python_install_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
        file(TO_CMAKE_PATH "${_python_install_dir}" python_install_dir)
    else()
        file(TO_CMAKE_PATH "${PYCLINGO_INSTALL_DIR}" python_install_dir)
    endif()
    message(STATUS "Python module path: ${python_install_dir}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/INSTALLER" "pip\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/METADATA" "Metadata-Version: 1.1\nName: clingo\nVersion: ${CLINGO_VERSION}\n")
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/INSTALLER" "${CMAKE_CURRENT_BINARY_DIR}/METADATA"
        DESTINATION "${python_install_dir}/clingo-${CLINGO_VERSION}.dist-info")
    install(DIRECTORY stubs/ DESTINATION "${python_install_dir}/clingo-stubs")
    install(
        TARGETS pyclingo
        EXPORT clingo-targets
        RUNTIME DESTINATION "${python_install_dir}"
        LIBRARY DESTINATION "${python_install_dir}")
endif()

if(CLINGO_BUILD_APP)
    # the embedded library
    add_library(clingo-python-embed STATIC)
    target_sources(clingo-python-embed PRIVATE "include/embed.h" "src/embed.cc")
    clingo_target_properties(TARGETS clingo-python-embed FOLDER lib TYPE extra)
    target_include_directories(clingo-python-embed PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    target_compile_definitions(clingo-python-embed PUBLIC CLINGO_PYTHON_ENABLED)
    target_link_libraries(clingo-python-embed PRIVATE pybind11::embed clingo clingo-python)

    target_compile_definitions(pyclingo PUBLIC CLINGO_PYTHON_EMBED)
endif()

if(CLINGO_BUILD_TESTS)
    pytest_discover_tests(
        test_pyclingo
        PYTHON_PATH_PREPEND $<TARGET_FILE_DIR:pyclingo>
        LIBRARY_PATH_PREPEND $<TARGET_FILE_DIR:clingo>
        DEPENDS pyclingo)
endif()
