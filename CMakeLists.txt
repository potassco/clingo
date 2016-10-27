cmake_minimum_required(VERSION 3.1)
project(CLASP VERSION 3.2.0 LANGUAGES CXX)
# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# NOTE: assumes that submodule has been initialized
add_subdirectory(clasp)
