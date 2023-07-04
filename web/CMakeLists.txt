#set(CMAKE_EXECUTABLE_SUFFIX ".html")

add_executable(web "main.cc")
target_link_libraries(web PRIVATE parsertest foonathan::lexy::core)
target_include_directories(web PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
set_target_properties(web PROPERTIES LINK_FLAGS "-s EXPORTED_RUNTIME_METHODS=ccall,cwrap")
set_target_properties(web PROPERTIES OUTPUT_NAME "parser")
