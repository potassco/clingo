set(source_path "${CLINGO_SOURCE_DIR}/libclingo/tests")
# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${source_path}/catch.hpp"
    "${source_path}/tests.hh"
    "${source_path}/symbol.cc"
    "${source_path}/ast.cc"
    "${source_path}/main.cc"
    "${source_path}/variant.cc"
    "${source_path}/clingo.cc"
    "${source_path}/propagator.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]
add_executable(test_clingo ${source})
target_link_libraries(test_clingo PRIVATE libclingo)
target_include_directories(test_clingo PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(test_clingo PROPERTIES FOLDER test)
if(MSVC)
    target_compile_definitions(test_clingo PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()
add_test(NAME test_clingo COMMAND test_clingo)
