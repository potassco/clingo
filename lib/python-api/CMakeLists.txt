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
    "src/collection.cc"
    "src/collection.hh"
    "src/config.cc"
    "src/config.hh"
    "src/control.cc"
    "src/control.hh"
    "src/core.cc"
    "src/core.hh"
    "src/embed.cc"
    "src/ground.cc"
    "src/ground.hh"
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
    "src/util.cc"
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
# the bindings use the limited API of Python 3.11 and the corresponding
# stable ABI if FindPython provides it (Development.SABIModule)
if(TARGET Python::SABIModule)
    set(pyclingo_python_target Python::SABIModule)
    set(pyclingo_sabi USE_SABI 3.11)
else()
    set(pyclingo_python_target Python::Module)
    set(pyclingo_sabi "")
endif()
target_compile_definitions(clingo-python
    PRIVATE CLINGO_PYTHON_VERSION="${Python_VERSION}"
    PUBLIC Py_LIMITED_API=0x030b0000)
target_link_libraries(clingo-python PRIVATE clingo ${pyclingo_python_target})

# the standalone module
Python_add_library(pyclingo MODULE WITH_SOABI ${pyclingo_sabi} "src/module.cc")
# FindPython may pick the abi3t suffix for Python 3.15 and later but the
# bindings target abi3, which is only compatible with GIL-enabled builds
get_target_property(pyclingo_suffix pyclingo SUFFIX)
if(pyclingo_sabi AND pyclingo_suffix AND NOT Python_FREE_THREADED)
    string(REPLACE ".abi3t" ".abi3" pyclingo_suffix "${pyclingo_suffix}")
    set_target_properties(pyclingo PROPERTIES SUFFIX "${pyclingo_suffix}")
endif()
set_target_properties(pyclingo PROPERTIES
    IMPORT_LIBRARY "pyclingo.lib"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
clingo_target_properties(TARGETS pyclingo FOLDER lib SUBDIR python)
set_target_properties(pyclingo PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(pyclingo PRIVATE clingo clingo-python)
if(CLINGO_USE_VERSION_SCRIPT AND UNIX AND NOT APPLE)
    set_target_properties(pyclingo PROPERTIES LINK_FLAGS "-Wl,--version-script='${CMAKE_SOURCE_DIR}/cmake/pyclingo.map'")
endif()
if(CLINGO_INSTALL_WHEEL OR CLINGO_INSTALL_DEFAULT)
    file(GLOB_RECURSE stub_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/stubs/*.pyi")
    set(stubs_dir "${CMAKE_CURRENT_BINARY_DIR}/clingo-stubs")
    add_custom_command(
        OUTPUT "${stubs_dir}.stamp"
        COMMAND "${Python_EXECUTABLE}" "${CLINGO_SOURCE_DIR}/scripts/stubs.py"
            --stubs "${CMAKE_CURRENT_SOURCE_DIR}/stubs" strip --output "${stubs_dir}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${stubs_dir}.stamp"
        DEPENDS ${stub_files} "${CLINGO_SOURCE_DIR}/scripts/stubs.py"
        COMMENT "Stripping overload implementations from the stubs"
        VERBATIM)
    add_custom_target(pyclingo-stubs ALL DEPENDS "${stubs_dir}.stamp")
endif()
if(CLINGO_INSTALL_WHEEL)
    install(DIRECTORY "${stubs_dir}/" DESTINATION clingo-stubs)
    install(TARGETS pyclingo
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION ".")
    if(CLINGO_BUILD_SHARED AND UNIX AND NOT APPLE)
        set_target_properties(pyclingo
            PROPERTIES INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE)
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
    install(DIRECTORY "${stubs_dir}/" DESTINATION "${python_install_dir}/clingo-stubs")
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
    target_link_libraries(clingo-python-embed PRIVATE clingo clingo-python Python::Python)

    target_compile_definitions(clingo-python PUBLIC CLINGO_PYTHON_EMBED)
endif()

if(CLINGO_BUILD_TESTS)
    add_library(fake_theory MODULE "tests/fake_theory.c")
    clingo_target_properties(TARGETS fake_theory FOLDER lib SUBDIR python)
    set_target_properties(fake_theory PROPERTIES PREFIX "")
    target_link_libraries(fake_theory PRIVATE clingo)

    pytest_discover_tests(
        test_pyclingo
        PYTHON_PATH_PREPEND
            $<TARGET_FILE_DIR:pyclingo>
            "${CLINGO_SOURCE_DIR}/third_party/pytest-cmake/src"
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:clingo>
        ENVIRONMENT
            PYCLINGO_FAKE_THEORY=$<TARGET_FILE:fake_theory>
        DEPENDS pyclingo fake_theory)
endif()
