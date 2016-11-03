enable_testing()

add_test(NAME test_clingo_app COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/run.py run)
set_tests_properties(test_clingo_app PROPERTIES DEPENDS clingo)
