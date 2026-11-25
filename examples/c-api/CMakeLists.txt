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
    "ground.c"
    "model.c"
    "observe.c"
    "profile.c"
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
    get_filename_component(name ${file} NAME_WE)
    set(target "example-c-${name}")
    add_executable(${target})
    target_sources(${target} PRIVATE ${file})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name})
    clingo_target_properties(TARGETS ${target} FOLDER example/c SUBDIR examples/c)
    target_link_libraries(${target} PRIVATE clingo)
endforeach()
