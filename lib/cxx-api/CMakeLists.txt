# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo
    "include/clingo/app.hh"
    "include/clingo/ast.hh"
    "include/clingo/backend.hh"
    "include/clingo/base.hh"
    "include/clingo/config.hh"
    "include/clingo/control.hh"
    "include/clingo/core.hh"
    "include/clingo/observe.hh"
    "include/clingo/propagate.hh"
    "include/clingo/script.hh"
    "include/clingo/solve.hh"
    "include/clingo/stats.hh"
    "include/clingo/symbol.hh"
    "include/clingo/theory.hh")
source_group("${ide_header_group}\\clingo" FILES ${header-group-clingo})
set(header-group-clingo-detail
    "include/clingo/detail/ast.hh")
source_group("${ide_header_group}\\clingo\\detail" FILES ${header-group-clingo-detail})
set(header
    ${header-group-clingo}
    ${header-group-clingo-detail})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group
    "tests/app.cc"
    "tests/app_error.cc"
    "tests/asp.cc"
    "tests/aspif.cc"
    "tests/ast.cc"
    "tests/backend.cc"
    "tests/base.cc"
    "tests/cbs.hh"
    "tests/config.cc"
    "tests/control.cc"
    "tests/core.cc"
    "tests/error.cc"
    "tests/incremental.cc"
    "tests/lp.hh"
    "tests/observe.cc"
    "tests/propagate.cc"
    "tests/script.cc"
    "tests/solve.cc"
    "tests/stats.cc"
    "tests/symbol.cc"
    "tests/tempfile.hh"
    "tests/write_aspif.cc")
source_group("${ide_test_group}" FILES ${test-group})
set(test
    ${test-group})
# ]]]

list(FILTER test EXCLUDE REGEX "/(app|app_error)\\.cc$")

add_library(clingo-cxx-api INTERFACE)
target_sources(clingo-cxx-api PRIVATE ${header})
clingo_target_properties(TARGETS clingo-cxx-api FOLDER lib TYPE default)
target_include_directories(clingo-cxx-api INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(clingo-cxx-api INTERFACE clingo)
if (CLINGO_INSTALL_DEFAULT)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hh")
elseif (CLINGO_INSTALL_WHEEL AND CLINGO_BUILD_SHARED)
    install(DIRECTORY include/ DESTINATION clingo-include FILES_MATCHING PATTERN "*.hh")
endif()

if(CLINGO_BUILD_TESTS)
    add_executable(test_clingo-cxx-api)
    target_sources(test_clingo-cxx-api PRIVATE ${test})
    clingo_target_properties(TARGETS test_clingo-cxx-api FOLDER test SUBDIR tests)
    target_include_directories(test_clingo-cxx-api PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_clingo-cxx-api PRIVATE clingo-cxx-api Catch2::Catch2WithMain)
    if (EMSCRIPTEN)
        target_link_options(test_clingo-cxx-api PRIVATE "SHELL:--preload-file=${CMAKE_SOURCE_DIR}/lib/python-api/tests/resources@/lib/python-api/tests/resources")
        get_target_property(test_clingo-cxx_API_BINARY_DIR test_clingo-cxx-api RUNTIME_OUTPUT_DIRECTORY)
        catch_discover_tests(test_clingo-cxx-api WORKING_DIRECTORY "${test_clingo-cxx_API_BINARY_DIR}" DL_PATH "$<TARGET_FILE_DIR:clingo>")
    else()
        catch_discover_tests(test_clingo-cxx-api WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.." DL_PATH "$<TARGET_FILE_DIR:clingo>")
    endif()

    add_executable(test_clingo-cxx-app)
    target_sources(test_clingo-cxx-app PRIVATE "tests/app.cc")
    clingo_target_properties(TARGETS test_clingo-cxx-app FOLDER test SUBDIR tests)
    target_include_directories(test_clingo-cxx-app PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_clingo-cxx-app PRIVATE clingo-cxx-api Catch2::Catch2WithMain)
    catch_discover_tests(test_clingo-cxx-app DL_PATH "$<TARGET_FILE_DIR:clingo>")

    add_executable(test_clingo-cxx-app-error)
    target_sources(test_clingo-cxx-app-error PRIVATE "tests/app_error.cc")
    clingo_target_properties(TARGETS test_clingo-cxx-app-error FOLDER test SUBDIR tests)
    target_include_directories(test_clingo-cxx-app-error PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_clingo-cxx-app-error PRIVATE clingo-cxx-api)

    function(add_clingo_error_test error)
        string(SUBSTRING ${error} 0 1 arg)
        set(test_name "test_clingo-cxx-app-error-${error}")
        add_test(NAME ${test_name} COMMAND test_clingo-cxx-app-error ${arg})
        set_tests_properties(${test_name} PROPERTIES
            PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): ${error}")
    endfunction()

    set(CLINGO_ERROR_WORDS main print register validate option invalid)
    foreach(error IN LISTS CLINGO_ERROR_WORDS)
        add_clingo_error_test(${error})
    endforeach()
endif()
