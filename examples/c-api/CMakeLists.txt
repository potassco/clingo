# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "app.c"
    "ast.c"
    "backend.c"
    "base-atoms.c"
    "base-theory.c"
    "config.c"
    "control.c"
    "model.c"
    "propagate.c"
    "solve.c"
    "stats.c"
    "symbol.c"
    "theory.c"
    "version.c")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

foreach(file ${source})
    get_filename_component(target ${file} NAME_WE)
    add_executable(${target} ${file})
    target_sources(${target} PRIVATE ${file})
    clingo_target_properties(${target} example examples)
    target_link_libraries(${target} PRIVATE clingo)
endforeach()
