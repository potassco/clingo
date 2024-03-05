add_executable(version "version.c")
target_link_libraries(version PRIVATE clingo)

add_executable(symbol "symbol.c")
target_link_libraries(symbol PRIVATE clingo)

add_executable(control "control.c")
target_link_libraries(control PRIVATE clingo)

add_executable(ast "ast.c")
target_link_libraries(ast PRIVATE clingo)
