add_library(gringo_control
    "src/grounder.cc"
    "src/theory.cc"
    "src/condlit.cc"
    "src/aggregate.cc"
    "src/statement.cc"
    "src/term.cc")

target_link_libraries(gringo_control PUBLIC gringo_input gringo_ground gringo_output)
target_include_directories(gringo_control PUBLIC "include")
set_property(TARGET gringo_control PROPERTY POSITION_INDEPENDENT_CODE ON)

if (PARSER_BUILD_TESTS)
    add_executable(test_control
        "tests/text.cc")
    target_link_libraries(test_control PRIVATE gringo_control Catch2::Catch2WithMain)
    target_include_directories(test_control PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    set_target_properties(test_control PROPERTIES FOLDER test_control)
    add_test(NAME test_control COMMAND test_control)
endif()
