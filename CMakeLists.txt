cmake_minimum_required(VERSION 3.1)

project(PARSERTEST VERSION "1.0" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CATCH_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "" FORCE)

enable_testing()

add_subdirectory(third_party/Catch2)
add_subdirectory(third_party/lexy)
add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(tests)
