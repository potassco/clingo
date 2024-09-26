add_library(gringo_ground
    "src/assignment_aggregate.cc"
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
    )

target_link_libraries(gringo_ground PUBLIC gringo_core)
if (PARSER_PROFILE)
    target_compile_definitions(gringo_ground PUBLIC "PARSER_PROFILE")
    target_link_libraries(gringo_ground PUBLIC Gperftools::Profiler)
endif()
target_include_directories(gringo_ground PUBLIC "include")
set_property(TARGET gringo_ground PROPERTY POSITION_INDEPENDENT_CODE ON)

if (PARSER_BUILD_TESTS)
    add_executable(test_ground
        "tests/matcher.cc")
    target_link_libraries(test_ground PRIVATE gringo_ground Catch2::Catch2WithMain)
    target_include_directories(test_ground PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_ground PROPERTIES FOLDER test_ground)
    add_test(NAME test_ground COMMAND test_ground)
endif()
