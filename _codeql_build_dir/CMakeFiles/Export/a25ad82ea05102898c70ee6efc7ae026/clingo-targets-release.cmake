#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "clingo::clingo" for configuration "Release"
set_property(TARGET clingo::clingo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(clingo::clingo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libclingo.so"
  IMPORTED_SONAME_RELEASE "libclingo.so"
  )

list(APPEND _cmake_import_check_targets clingo::clingo )
list(APPEND _cmake_import_check_files_for_clingo::clingo "${_IMPORT_PREFIX}/lib/libclingo.so" )

# Import target "clingo::pyclingo" for configuration "Release"
set_property(TARGET clingo::pyclingo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(clingo::pyclingo PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELEASE ""
  IMPORTED_LOCATION_RELEASE "/usr/local/local/lib/python3.12/dist-packages/clingo.cpython-312-x86_64-linux-gnu.so"
  IMPORTED_NO_SONAME_RELEASE "TRUE"
  )

list(APPEND _cmake_import_check_targets clingo::pyclingo )
list(APPEND _cmake_import_check_files_for_clingo::pyclingo "/usr/local/local/lib/python3.12/dist-packages/clingo.cpython-312-x86_64-linux-gnu.so" )

# Import target "clingo::clingo-app" for configuration "Release"
set_property(TARGET clingo::clingo-app APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(clingo::clingo-app PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/clingo"
  )

list(APPEND _cmake_import_check_targets clingo::clingo-app )
list(APPEND _cmake_import_check_files_for_clingo::clingo-app "${_IMPORT_PREFIX}/bin/clingo" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
