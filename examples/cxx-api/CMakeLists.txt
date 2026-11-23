# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "app.cc"
    "ast.cc"
    "backend.cc"
    "base-atoms.cc"
    "base-theory.cc"
    "config.cc"
    "control.cc"
    "ground.cc"
    "model.cc"
    "profile.cc"
    "propagate.cc"
    "solve.cc"
    "stats.cc"
    "symbol.cc"
    "theory.cc"
    "version.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

foreach(file ${source})
    get_filename_component(name ${file} NAME_WE)
    set(target "example-cxx-${name}")
    add_executable(${target})
    target_sources(${target} PRIVATE ${file})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name})
    clingo_target_properties(TARGETS ${target} FOLDER example/cxx SUBDIR examples/cxx)
    target_link_libraries(${target} PRIVATE clingo-cxx-api)
endforeach()
