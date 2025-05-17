add_library(clingo_cxx-api INTERFACE)
add_library(clingo::cxx-api ALIAS clingo_cxx-api)
clingo_target_properties(clingo_cxx-api lib/cxx-api)
target_include_directories(clingo_cxx-api INTERFACE "include")
target_link_libraries(clingo_cxx-api INTERFACE clingo)

if(PARSER_BUILD_TESTS)
    add_executable(test_cxx-api
        "tests/ast.cc"
        "tests/aspif.cc"
        "tests/base.cc"
        "tests/control.cc"
        "tests/symbol.cc"
        "tests/solve.cc"
    )
    clingo_target_properties(test_cxx-api test/cxx-api tests)
    target_include_directories(test_cxx-api PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_cxx-api PRIVATE clingo::cxx-api Catch2::Catch2WithMain)
    catch_discover_tests(test_cxx-api)

    add_executable(test_cxx-app "tests/app.cc")
    clingo_target_properties(test_cxx-app test/cxx-api tests)
    target_include_directories(test_cxx-app PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_cxx-app PRIVATE clingo::cxx-api)
    add_test(NAME test_cxx-app COMMAND test_cxx-app)
endif()
