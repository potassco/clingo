enable_testing()

find_program(CLINGO_PYTHON_EXE NAMES "python")

if (CLINGO_PYTHON_EXE)
    add_test(NAME test_clingo_app COMMAND ${CLINGO_PYTHON_EXE} "${CMAKE_CURRENT_SOURCE_DIR}/run.py" -c $<TARGET_FILE:clingo> run)
endif()
