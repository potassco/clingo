add_library(clingo
    "src/ast.cc"
    "src/main.cc"
    "src/ast_yaml.cc"
    "src/control.cc"
    "src/core.cc"
    "src/symbol.cc"
    )
target_link_libraries(clingo PRIVATE clingo_control CLI11)
target_include_directories(clingo PUBLIC "include")
set_property(TARGET clingo PROPERTY POSITION_INDEPENDENT_CODE ON)
