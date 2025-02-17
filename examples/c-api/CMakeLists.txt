set(sources
    ast.c
    base-atoms.c
    base-theory.c
    config.c
    control.c
    model.c
    solve.c
    stats.c
    symbol.c
    version.c
)

foreach(source ${sources})
    get_filename_component(target ${source} NAME_WE)
    add_executable(${target} ${source})
    clingo_target_properties(${target} examples examples)
    target_link_libraries(${target} PRIVATE clingo)
endforeach()
