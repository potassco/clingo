add_library(clingo
    "src/ast.cc"
    "src/ast_yaml.cc"
    "src/base.cc"
    "src/control.cc"
    "src/core.cc"
    "src/main.cc"
    "src/model.cc"
    "src/script.cc"
    "src/solve.cc"
    "src/symbol.cc"
    )
target_link_libraries(clingo PRIVATE clingo::control CLI11)
target_include_directories(clingo PUBLIC "include")
set_property(TARGET clingo PROPERTY POSITION_INDEPENDENT_CODE ON)
