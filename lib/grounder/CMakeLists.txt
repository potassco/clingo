add_library(gringo_grounder
    "src/grounder.cc"
    "src/theory.cc"
    "src/condlit.cc"
    "src/aggregate.cc"
    "src/term.cc")

target_link_libraries(gringo_grounder PUBLIC gringo_input gringo_ground gringo_output)
target_include_directories(gringo_grounder PUBLIC "include")
set_property(TARGET gringo_grounder PROPERTY POSITION_INDEPENDENT_CODE ON)

if (PARSER_BUILD_TESTS)
    add_executable(test_grounder
        "tests/text.cc")
    target_link_libraries(test_grounder PRIVATE gringo_grounder Catch2::Catch2WithMain)
    target_include_directories(test_grounder PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_grounder PROPERTIES FOLDER test_grounder)
    add_test(NAME test_grounder COMMAND test_grounder)
endif()
