# functionality shared between embedded library and standalone module
add_library(pyclingo_lib
    src/ast.cc
    src/ast2.cc
    src/clingo.cc
    src/core.cc
    src/control.cc
    src/script.cc
    src/symbol.cc
    )
target_link_libraries(pyclingo_lib PRIVATE pybind11::pybind11 clingo)
set_property(TARGET pyclingo_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

# the standalone module
pybind11_add_module(pyclingo_module src/module.cc)
set_target_properties(pyclingo_module PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(pyclingo_module PRIVATE pyclingo_lib)

# the embedded library
add_library(pyclingo_embed src/embed.cc)
target_link_libraries(pyclingo_embed PRIVATE pybind11::embed pyclingo_lib)

if (PYTHON_EXECUTABLE AND NOT Python_EXECUTABLE)
    set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
endif()

if (Python_EXECUTABLE)
    add_test(NAME test_pyclingo COMMAND ${Python_EXECUTABLE} -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set_property(TEST test_pyclingo APPEND PROPERTY ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pyclingo_module>")
endif()
