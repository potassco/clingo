enable_testing()

if (Python_EXECUTABLE)
    if (CLASP_BUILD_WITH_THREADS)
        set(solver-options "-t8")
    endif()
    add_test(NAME test_clingo_app COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/run.py" -c $<TARGET_FILE:clingo> run -- ${solver-options})
endif()
