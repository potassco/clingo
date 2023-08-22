add_executable(parser "main.cc")
target_link_libraries(parser PRIVATE parsertest)
target_include_directories(parser PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
