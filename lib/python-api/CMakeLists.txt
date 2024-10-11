pybind11_add_module(pyclingo
    src/ast.cc
    src/ast2.cc
    src/clingo.cc
    src/core.cc
    src/symbol.cc
    )
set_target_properties(pyclingo PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(pyclingo PRIVATE clingo)

if (PYTHON_EXECUTABLE AND NOT Python_EXECUTABLE)
    set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
endif()

if (Python_EXECUTABLE)
    add_test(NAME test_pyclingo COMMAND ${Python_EXECUTABLE} -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set_property(TEST test_pyclingo APPEND PROPERTY ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pyclingo>")
endif()
