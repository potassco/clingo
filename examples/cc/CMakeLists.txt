add_executable(cc-solve-iteratively "solve-iteratively.cc")
target_link_libraries(cc-solve-iteratively PRIVATE libclingo)
set_target_properties(cc-solve-iteratively PROPERTIES FOLDER "exe/clingo-cc-examples")
