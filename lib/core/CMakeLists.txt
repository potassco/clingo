# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo-core
    "include/clingo/core/backend.hh"
    "include/clingo/core/core.hh"
    "include/clingo/core/fstring.hh"
    "include/clingo/core/location.hh"
    "include/clingo/core/logger.hh"
    "include/clingo/core/number.hh"
    "include/clingo/core/output.hh"
    "include/clingo/core/symbol.hh")
source_group("${ide_header_group}\\clingo\\core" FILES ${header-group-clingo-core})
set(header
    ${header-group-clingo-core})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "src/core.cc"
    "src/fstring.cc"
    "src/number.cc"
    "src/symbol.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group
    "tests/number.cc"
    "tests/roles.cc"
    "tests/symbol.cc")
source_group("${ide_test_group}" FILES ${test-group})
set(test
    ${test-group})
# ]]]

add_library(clingo-core STATIC)
target_sources(clingo-core PRIVATE ${header} ${source})
clingo_target_properties(TARGETS clingo-core FOLDER lib TYPE extra)
target_include_directories(clingo-core
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo-core PUBLIC clingo-util tsl::hopscotch_map tsl::ordered_map PRIVATE imath::imath)

if(CLINGO_BUILD_TESTS)
    add_executable(test_clingo-core)
    target_sources(test_clingo-core PRIVATE ${test})
    clingo_target_properties(TARGETS test_clingo-core FOLDER test SUBDIR tests)
    target_include_directories(test_clingo-core PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_clingo-core PRIVATE clingo-core Catch2::Catch2WithMain)
    catch_discover_tests(test_clingo-core DL_PATH "$<TARGET_FILE_DIR:clingo>")
endif()
