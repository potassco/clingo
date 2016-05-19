project (gringo)

cmake_minimum_required(VERSION 2.4)
cmake_policy(SET CMP0005 OLD)

include (cmake/pch.cmake)
include (cmake/oos.cmake)
include (cmake/re2c.cmake)
include (cmake/lemon.cmake)

ENSURE_OUT_OF_SOURCE_BUILD("you have to create an out-of-source build")

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
SET(CPACK_PACKAGE_VERSION "2.0.5")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "5")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-source")
SET(CPACK_SOURCE_IGNORE_FILES "/build/" "/examples/" "/bin/" "/\\\\.svn/" "release.sh" "TODO" "CMakeLists.txt.user")
SET(CPACK_SOURCE_GENERATOR "TGZ")
include (CPack)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake CACHE INTERNAL "" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE)

IF(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /D \"_DEBUG\" /D \"_SCL_SECURE_NO_WARNINGS\" /D \"_UNICODE\" /D \"UNICODE\" /Gm /EHsc /RTC1 /MDd /W3 /nologo /c /ZI /TP" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL /D \"NDEBUG\" /D \"_UNICODE\" /D \"UNICODE\" /FD /EHsc /MD /Gy /W3 /nologo /c /Zi /TP" CACHE STRING "Flags used by the compiler during release builds" FORCE)
	ADD_DEFINITIONS(/D \"HAS_TR1_UNORDERED=1\" /D \"NS_TR1=std::tr1\")
ENDIF()

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()
set(GRINGO_TYPE "gringo" CACHE STRING "Choose whether to build gringo, clingo or iclingo.")

# add macros for version information
ADD_DEFINITIONS(-DPACK_VERSION="\\"${CPACK_PACKAGE_VERSION}\\"")
ADD_DEFINITIONS(-DGRINGO_VERSION="\\"${CPACK_PACKAGE_VERSION}\\"")
ADD_DEFINITIONS(-DCLASP_VERSION="\\"1.3.2\\"")

if(${GRINGO_TYPE} STREQUAL "clingo")
	ADD_DEFINITIONS(-DWITH_CLASP)
	ADD_DEFINITIONS(-DEXECUTABLE="\\"clingo\\"")
	add_subdirectory (libclasp)
endif()

if(${GRINGO_TYPE} STREQUAL "iclingo")
	ADD_DEFINITIONS(-DWITH_CLASP)
	ADD_DEFINITIONS(-DWITH_ICLASP)
	ADD_DEFINITIONS(-DEXECUTABLE="\\"iclingo\\"")
	add_subdirectory (libclasp)
endif()

if(${GRINGO_TYPE} STREQUAL "gringo")
	ADD_DEFINITIONS(-DEXECUTABLE="\\"gringo\\"")
endif()

add_subdirectory (app)
add_subdirectory (lemon)
add_subdirectory (libgringo)
add_subdirectory (libprogram_opts)

