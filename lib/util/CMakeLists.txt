# [[[header: include
set(ide_header_group "Header Files")
set(header-group-clingo-util
    "include/clingo/util/algorithm.hh"
    "include/clingo/util/checked_math.hh"
    "include/clingo/util/debug.hh"
    "include/clingo/util/enum.hh"
    "include/clingo/util/enumerate.hh"
    "include/clingo/util/graph.hh"
    "include/clingo/util/hash.hh"
    "include/clingo/util/immutable_array.hh"
    "include/clingo/util/immutable_value.hh"
    "include/clingo/util/index_sequence.hh"
    "include/clingo/util/interval_set.hh"
    "include/clingo/util/macro.hh"
    "include/clingo/util/optional.hh"
    "include/clingo/util/ordered_map.hh"
    "include/clingo/util/ordered_set.hh"
    "include/clingo/util/print.hh"
    "include/clingo/util/record.hh"
    "include/clingo/util/small_vector.hh"
    "include/clingo/util/string.hh"
    "include/clingo/util/type_traits.hh"
    "include/clingo/util/unordered_map.hh"
    "include/clingo/util/unordered_set.hh")
source_group("${ide_header_group}\\clingo\\util" FILES ${header-group-clingo-util})
set(header
    ${header-group-clingo-util})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group
    "tests/interval_set.cc"
    "tests/record.cc")
source_group("${ide_test_group}" FILES ${test-group})
set(test
    ${test-group})
# ]]]

add_library(clingo_util INTERFACE)
target_sources(clingo_util PRIVATE ${header})
add_library(clingo::util ALIAS clingo_util)
clingo_target_properties(clingo_util lib)
target_include_directories(clingo_util
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(clingo_util INTERFACE tsl::hopscotch_map tsl::ordered_map)
clingo_install_target(clingo_util extra)

if (CLINGO_BUILD_TESTS)
    add_executable(test_util)
    target_sources(test_util PRIVATE ${test})
    clingo_target_properties(test_util test tests tests)
    target_include_directories(test_util PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(test_util PRIVATE clingo::util Catch2::Catch2WithMain)
    catch_discover_tests(test_util)
endif()
