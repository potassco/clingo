add_executable(tests "main.cc" "pegtl.cc")
target_link_libraries(tests PRIVATE pegtl foonathan::lexy::core Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tests PROPERTIES FOLDER test)
add_test(NAME tests COMMAND tests)

