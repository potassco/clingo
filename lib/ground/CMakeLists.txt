add_library(clingo_ground STATIC
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
clingo_target_properties(clingo_ground lib/ground)
target_include_directories(clingo_ground
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo_ground PUBLIC clingo::core)
if (CLINGO_PROFILE)
    target_compile_definitions(clingo_ground PUBLIC "CLINGO_PROFILE")
    target_link_libraries(clingo_ground PUBLIC Gperftools::Profiler)
endif()
clingo_install_target(clingo_ground extra)

if (CLINGO_BUILD_TESTS)
    add_executable(test_ground
        "tests/matcher.cc")
    clingo_target_properties(test_ground test/ground tests)
    target_include_directories(test_ground PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_ground PRIVATE clingo::ground Catch2::Catch2WithMain)
    catch_discover_tests(test_ground)
endif()
