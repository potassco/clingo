pybind11_add_module(pyclingo src/clingo.cc)
set_target_properties(pyclingo PROPERTIES OUTPUT_NAME "clingo")
target_link_libraries(pyclingo PRIVATE clingo)
