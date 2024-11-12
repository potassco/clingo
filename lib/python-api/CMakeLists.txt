# functionality shared between embedded library and standalone module
add_library(clingo_python
    src/ast.cc
    src/clingo.cc
    src/core.cc
    src/control.cc
    src/script.cc
    src/symbol.cc
    )
target_link_libraries(clingo_python PRIVATE pybind11::pybind11 clingo)
set_property(TARGET clingo_python PROPERTY POSITION_INDEPENDENT_CODE ON)

# the standalone module
pybind11_add_module(clingo_python_module src/module.cc)
set_target_properties(clingo_python_module PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(clingo_python_module PRIVATE clingo clingo_python)

# the embedded library
add_library(clingo_python_embed src/embed.cc)
add_library(clingo::python ALIAS clingo_python_embed)
target_include_directories(clingo_python_embed PUBLIC "include")
target_link_libraries(clingo_python_embed PRIVATE pybind11::embed clingo clingo_python)
target_compile_definitions(clingo_python_embed PRIVATE CLINGO_PYTHON_VERSION="${Python_VERSION}")
if (PYTHON_EXECUTABLE AND NOT Python_EXECUTABLE)
    set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
endif()

if (Python_EXECUTABLE)
    add_test(NAME test_pyclingo COMMAND ${Python_EXECUTABLE} -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set_property(TEST test_pyclingo APPEND PROPERTY ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:clingo_python_module>")
endif()
