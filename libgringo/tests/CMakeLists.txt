set(source_path "${CLINGO_SOURCE_DIR}/libgringo/tests")
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/catch.hpp"
    "${source_path}/graph.cc"
    "${source_path}/intervals.cc"
    "${source_path}/main.cc"
    "${source_path}/python.cc"
    "${source_path}/safetycheck.cc"
    "${source_path}/symbol.cc"
    "${source_path}/term.cc"
    "${source_path}/term_helper.hh"
    "${source_path}/tests.hh"
    "${source_path}/unique_vec.cc"
    "${source_path}/utility.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source-group-ground
    "${source_path}/ground/dependency.cc"
    "${source_path}/ground/grounder_helper.hh"
    "${source_path}/ground/instantiation.cc"
    "${source_path}/ground/literal.cc"
    "${source_path}/ground/program.cc"
    "${source_path}/ground/theory.cc")
source_group("${ide_source_group}\\ground" FILES ${source-group-ground})
set(source-group-input
    "${source_path}/input/aggregate.cc"
    "${source_path}/input/aggregate_helper.hh"
    "${source_path}/input/lit_helper.hh"
    "${source_path}/input/literal.cc"
    "${source_path}/input/nongroundgrammar.cc"
    "${source_path}/input/nongroundlexer.cc"
    "${source_path}/input/parse_term.cc"
    "${source_path}/input/program.cc"
    "${source_path}/input/programbuilder.cc")
source_group("${ide_source_group}\\input" FILES ${source-group-input})
set(source-group-output
    "${source_path}/output/aspcomp13.cc"
    "${source_path}/output/incremental.cc"
    "${source_path}/output/lparse.cc"
    "${source_path}/output/solver_helper.hh"
    "${source_path}/output/theory.cc"
    "${source_path}/output/warnings.cc")
source_group("${ide_source_group}\\output" FILES ${source-group-output})
set(source
    ${source-group}
    ${source-group-ground}
    ${source-group-input}
    ${source-group-output})
# ]]]
add_executable(test_gringo ${source})
target_link_libraries(test_gringo PRIVATE libgringo libclasp)
target_include_directories(test_gringo PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CLINGO_BINARY_DIR}/libgringo/src>)

if(MSVC)
    target_compile_definitions(test_gringo PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()
set_target_properties(test_gringo PROPERTIES FOLDER test)

add_test(NAME test_gringo COMMAND test_gringo)
