add_library(clingo_ground
    "src/assignment_aggregate.cc"
    "src/base.cc"
    "src/body_aggregate.cc"
    "src/condlit.cc"
    "src/disjunction.cc"
    "src/head_aggregate.cc"
    "src/instantiator.cc"
    "src/literal.cc"
    "src/matcher.cc"
    "src/program.cc"
    "src/statement.cc"
    "src/term.cc"
    "src/theory_atom.cc"
    "src/theory_term.cc"
    )
add_library(clingo::ground ALIAS clingo_ground)

target_link_libraries(clingo_ground PUBLIC clingo::core)
if (PARSER_PROFILE)
    target_compile_definitions(clingo_ground PUBLIC "PARSER_PROFILE")
    target_link_libraries(clingo_ground PUBLIC Gperftools::Profiler)
endif()
target_include_directories(clingo_ground PUBLIC "include")
set_property(TARGET clingo_ground PROPERTY POSITION_INDEPENDENT_CODE ON)

if (PARSER_BUILD_TESTS)
    add_executable(test_ground
        "tests/matcher.cc")
    target_link_libraries(test_ground PRIVATE clingo::ground Catch2::Catch2WithMain)
    target_include_directories(test_ground PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_ground PROPERTIES FOLDER test_ground)
    if (PARSER_BUILD_WEB)
        add_test(NAME test_ground COMMAND test_ground)
    else()
        catch_discover_tests(test_ground)
    endif()
endif()
