add_library(clingo_cxx INTERFACE)
add_library(clingo::cxx ALIAS clingo_cxx)
clingo_target_properties(clingo_cxx lib/cxx)
target_include_directories(clingo_cxx INTERFACE "include")
target_link_libraries(clingo_cxx INTERFACE clingo)

if (PARSER_BUILD_TESTS)
    add_executable(test_cxx
        "tests/symbol.cc")
    clingo_target_properties(clingo_cxx test/cxx)
    target_include_directories(test_cxx PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_cxx PRIVATE clingo::cxx Catch2::Catch2WithMain)
    catch_discover_tests(test_cxx)
endif()
