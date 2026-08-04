cmake_minimum_required(VERSION 3.10)
project(LIB_POTASSCO VERSION 1.2.0 LANGUAGES CXX)
# enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(GNUInstallDirs)
option(LIB_POTASSCO_BUILD_TESTS       "whether or not to build tests"             OFF)
option(LIB_POTASSCO_BUILD_APP         "whether or not to build lpconvert tool"    ON)
option(LIB_POTASSCO_INSTALL_LIB       "whether or not to install libpotassco"     OFF)
option(LIB_POTASSCO_INSTALL_VERSIONED "whether to use a versioned install layout" OFF)

if (LIB_POTASSCO_INSTALL_LIB AND NOT CMAKE_INSTALL_LIBDIR)
	message(STATUS "LIBDIR no set - using lib")
	set(CMAKE_INSTALL_LIBDIR lib)
endif()

set(potassco_include_dest  "${CMAKE_INSTALL_INCLUDEDIR}")
set(potassco_library_dest  "${CMAKE_INSTALL_LIBDIR}")

if (LIB_POTASSCO_INSTALL_VERSIONED)
	string(APPEND potassco_include_dest "/potassco-${LIB_POTASSCO_VERSION}")
	string(APPEND potassco_library_dest "/potassco-${LIB_POTASSCO_VERSION}")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "No build type selected - using 'Release'")
	set(CMAKE_BUILD_TYPE "Release")
endif()

if (NOT MSVC)
	if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	endif()
	if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	endif()
	if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	endif()
else()
    set(VC_RELEASE_LINK_OPTIONS /LTCG)
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    ${VC_RELEASE_LINK_OPTIONS}")
	SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
	SET(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
endif()

add_subdirectory(src)
if(LIB_POTASSCO_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
if(LIB_POTASSCO_BUILD_APP)
	add_subdirectory(app)
endif()

# optional doc target
find_package(Doxygen)
if(DOXYGEN_FOUND)
	set(doxyfile "${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile")
	add_custom_target(doc_potassco
		COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doc"
		COMMENT "Generating documentation..."
		VERBATIM)
	set_target_properties(doc_potassco PROPERTIES FOLDER doc)
endif()

# export
if(LIB_POTASSCO_INSTALL_LIB)
	set(cmake_dest "${CMAKE_INSTALL_LIBDIR}/cmake/Potassco")
	if (LIB_POTASSCO_INSTALL_VERSIONED)
		string(APPEND cmake_dest "-${LIB_POTASSCO_VERSION}")
	endif()
	include(CMakePackageConfigHelpers)
	configure_package_config_file(
		${PROJECT_SOURCE_DIR}/cmake/PotasscoConfig.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/PotasscoConfig.cmake
		INSTALL_DESTINATION ${cmake_dest})
	write_basic_package_version_file(
		${CMAKE_CURRENT_BINARY_DIR}/PotasscoConfigVersion.cmake
		COMPATIBILITY SameMajorVersion)
	install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/PotasscoConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/PotasscoConfigVersion.cmake
		DESTINATION ${cmake_dest})
	install(EXPORT PotasscoTargets DESTINATION "${cmake_dest}")
endif()
