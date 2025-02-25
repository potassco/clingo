add_library(clingo
    "src/ast.cc"
    "src/ast_yaml.cc"
    "src/backend.cc"
    "src/base.cc"
    "src/config.cc"
    "src/control.cc"
    "src/core.cc"
    "src/main.cc"
    "src/model.cc"
    "src/observe.cc"
    "src/propagate.cc"
    "src/script.cc"
    "src/solve.cc"
    "src/stats.cc"
    "src/symbol.cc"
    )
clingo_target_properties(clingo "lib/clingo")
target_include_directories(clingo PUBLIC "include")
target_link_libraries(clingo PRIVATE clingo::control)
