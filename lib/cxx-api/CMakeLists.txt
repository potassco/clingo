add_library(clingo_cxx-api INTERFACE)
add_library(clingo::cxx-api ALIAS clingo_cxx-api)
clingo_target_properties(clingo_cxx-api lib/cxx-api)
target_include_directories(clingo_cxx-api INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(clingo_cxx-api INTERFACE clingo)

if (DEFINED SKBUILD AND PARSER_BUILD_SHARED)
    install(DIRECTORY include/ DESTINATION . FILES_MATCHING PATTERN "*.hh")
endif()
if (NOT DEFINED SKBUILD)
    install(TARGETS clingo_cxx-api
        EXPORT clingo-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hh")
endif()
if(PARSER_BUILD_TESTS)
    add_executable(test_cxx-api
        "tests/asp.cc"
        "tests/aspif.cc"
        "tests/ast.cc"
        "tests/backend.cc"
        "tests/base.cc"
        "tests/config.cc"
        "tests/control.cc"
        "tests/core.cc"
        "tests/error.cc"
        "tests/incremental.cc"
        "tests/observe.cc"
        "tests/propagate.cc"
        "tests/script.cc"
        "tests/solve.cc"
        "tests/stats.cc"
        "tests/symbol.cc"
        "tests/write_aspif.cc"
    )
    clingo_target_properties(test_cxx-api test/cxx-api tests)
    target_include_directories(test_cxx-api PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_cxx-api PRIVATE clingo::cxx-api Catch2::Catch2WithMain)
    if (EMSCRIPTEN)
        target_link_options(test_cxx-api PRIVATE "SHELL:--preload-file=${CMAKE_SOURCE_DIR}/lib/python-api/tests/resources@/lib/python-api/tests/resources")
        get_target_property(TEST_CXX_API_BINARY_DIR test_cxx-api RUNTIME_OUTPUT_DIRECTORY)
        catch_discover_tests(test_cxx-api WORKING_DIRECTORY "${TEST_CXX_API_BINARY_DIR}")
    else()
        catch_discover_tests(test_cxx-api WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../..")
    endif()

    add_executable(test_cxx-app "tests/app.cc")
    clingo_target_properties(test_cxx-app test/cxx-api tests)
    target_include_directories(test_cxx-app PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_cxx-app PRIVATE clingo::cxx-api Catch2::Catch2WithMain)
    catch_discover_tests(test_cxx-app)

    add_executable(test_cxx-app-error "tests/app_error.cc")
    clingo_target_properties(test_cxx-app-error test/cxx-api tests)
    target_include_directories(test_cxx-app-error PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_cxx-app-error PRIVATE clingo::cxx-api)

    function(add_clingo_error_test error)
        string(SUBSTRING ${error} 0 1 arg)
        set(test_name "test_cxx-app-error-${error}")
        add_test(NAME ${test_name} COMMAND test_cxx-app-error ${arg})
        set_tests_properties(${test_name} PROPERTIES
            PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): ${error}"
    )
    endfunction()

    set(CLINGO_ERROR_WORDS main print register validate option invalid)
    foreach(error IN LISTS CLINGO_ERROR_WORDS)
        add_clingo_error_test(${error})
    endforeach()
endif()
