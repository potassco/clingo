# functionality shared between embedded library and standalone module
add_library(clingo_python STATIC
    src/app.cc
    src/ast.cc
    src/backend.cc
    src/base.cc
    src/clingo.cc
    src/config.cc
    src/control.cc
    src/core.cc
    src/propagate.cc
    src/script.cc
    src/solve.cc
    src/stats.cc
    src/symbol.cc
    src/theory.cc
    )
clingo_target_properties(clingo_python "lib/pyclingo")
target_compile_definitions(clingo_python PRIVATE CLINGO_PYTHON_VERSION="${Python_VERSION}")
target_link_libraries(clingo_python PRIVATE pybind11::pybind11 clingo)

# the standalone module
pybind11_add_module(clingo_python_module src/module.cc)
clingo_target_properties(clingo_python_module "lib/pyclingo-module" "python")
set_target_properties(clingo_python_module PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(clingo_python_module PRIVATE clingo clingo_python)
if (CLINGO_USE_VERSION_SCRIPT AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set_target_properties(clingo_python_module PROPERTIES LINK_FLAGS "-Wl,--version-script='${CMAKE_SOURCE_DIR}/cmake/pyclingo.map'")
endif()
if (CLINGO_INSTALL_WHEEL)
    install(DIRECTORY stubs/ DESTINATION clingo-stubs)
    install(TARGETS clingo_python_module
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION ".")
    if (CLINGO_BUILD_SHARED)
        if (APPLE)
            set_target_properties(clingo_python_module
                PROPERTIES INSTALL_RPATH "@loader_path"
                BUILD_WITH_INSTALL_RPATH TRUE)
        elseif (UNIX)
            set_target_properties(clingo_python_module
                PROPERTIES INSTALL_RPATH "$ORIGIN"
                BUILD_WITH_INSTALL_RPATH TRUE)
        endif()
    endif()
elseif(CLINGO_INSTALL_DEFAULT)
    if (PYCLINGO_INSTALL_DIR STREQUAL "")
        if (PYCLINGO_INSTALL STREQUAL "user")
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
        set(python_install_dir "${PYCLINGO_INSTALL_DIR}")
    endif()
    MESSAGE(STATUS "Python module path: ${python_install_dir}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/INSTALLER" "pip\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/METADATA" "Metadata-Version: 1.1\nName: clingo\nVersion: ${CLINGO_VERSION}\n")
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/INSTALLER" "${CMAKE_CURRENT_BINARY_DIR}/METADATA"
        DESTINATION "${python_install_dir}/clingo-${CLINGO_VERSION}.dist-info")
    install(DIRECTORY stubs/ DESTINATION "${python_install_dir}/clingo-stubs")
    install(
        TARGETS clingo_python_module
        RUNTIME DESTINATION "${python_install_dir}"
        LIBRARY DESTINATION "${python_install_dir}")
endif()

# the embedded library
add_library(clingo_python_embed STATIC src/embed.cc)
add_library(clingo::python ALIAS clingo_python_embed)
clingo_target_properties(clingo_python_embed "lib/pyclingo-embed")
target_include_directories(clingo_python_embed PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_definitions(clingo_python_embed PUBLIC CLINGO_PYTHON_ENABLED)
target_link_libraries(clingo_python_embed PRIVATE pybind11::embed clingo clingo_python)
clingo_install_target(clingo_python_embed extra)

if (PYTEST_EXECUTABLE)
    pytest_discover_tests(
        test_pyclingo
        PYTHON_PATH_PREPEND
            $<TARGET_FILE_DIR:clingo_python_module>
        DEPENDS
            clingo_python_module
    )
endif()
