function(re2c_target_or_gen GRAMMAR)
    get_filename_component(directory "${GRAMMAR}" DIRECTORY)
    get_filename_component(filename "${GRAMMAR}" NAME_WE)
    get_filename_component(extension "${GRAMMAR}" EXT)
    set(bin_path "${CMAKE_CURRENT_BINARY_DIR}/${directory}")
    set(gen_path "${CMAKE_CURRENT_SOURCE_DIR}/gen/${directory}")
    file(MAKE_DIRECTORY "${bin_path}")
    if(RE2C_FOUND)
        if (extension STREQUAL ".xch")
            re2c_target(NAME ${filename} INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${GRAMMAR}" OUTPUT "${bin_path}/${filename}.hh" HEADER "${bin_path}/${filename}_h.hh" OPTIONS -c)
        else()
            re2c_target(NAME ${filename} INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${GRAMMAR}" OUTPUT "${bin_path}/${filename}.hh" HEADER "${bin_path}/${filename}_h.hh")
        endif()
        set("RE2C_${filename}_OUTPUT" ${RE2C_${filename}_OUTPUT} PARENT_SCOPE)
        set("RE2C_${filename}_HEADER" ${RE2C_${filename}_HEADER} PARENT_SCOPE)
        # add to gen target
        add_custom_target("gen${gen_count}" DEPENDS "${bin_path}/${filename}.hh")
        add_dependencies(gen "gen${gen_count}")
        math(EXPR gen_count "${gen_count}+1")
        set(gen_count "${gen_count}" PARENT_SCOPE)
    elseif (EXISTS "${gen_path}/${filename}.hh")
        file(COPY "${gen_path}/${filename}.hh" DESTINATION "${bin_path}")
        set("RE2C_${filename}_OUTPUT" "${bin_path}/${filename}.hh" PARENT_SCOPE)
        set("RE2C_${filename}_HEADER" "${bin_path}/${filename}_h.hh" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "re2c lexer generator required but not found")
    endif()
endfunction()

#[=======================================================================[.rst:
.. command:: clingo_target_properties

Set output properties for a target, including output directories and FOLDER property.

.. code-block:: cmake

   clingo_target_properties(target folder [binary_subdir])

``target``
  The name of the target to set properties for.

``folder``
  The folder name for organizing targets in IDEs.

``binary_subdir``
  Optional. Subdirectory within "bin" for placing binaries. If not provided, binaries are placed directly in "bin".

This function sets the following properties:
- RUNTIME_OUTPUT_DIRECTORY
- LIBRARY_OUTPUT_DIRECTORY
- ARCHIVE_OUTPUT_DIRECTORY
- PDB_OUTPUT_DIRECTORY
- FOLDER
- POSITION_INDEPENDENT_CODE

It handles both single-config and multi-config generators, using the GENERATOR_IS_MULTI_CONFIG property to determine the appropriate output structure.

Example usage:
.. code-block:: cmake

   add_executable(myapp main.cpp)
   clingo_target_properties(myapp "MyApps")

   add_library(mylib SHARED lib.cpp)
   clingo_target_properties(mylib "MyLibraries" "plugins")
#]=======================================================================]
function(clingo_target_properties target folder)
    set(binary_subdir "bin")
    if(${ARGC} GREATER 2)
        set(binary_subdir "bin/${ARGV2}")
    endif()
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    set(base_dir "${CMAKE_BINARY_DIR}")
    if (is_multi_config)
        set(base_dir "${base_dir}/$<CONFIG>")
    endif()
    set_target_properties("${target}" PROPERTIES
        FOLDER "${folder}"
        POSITION_INDEPENDENT_CODE ON
        RUNTIME_OUTPUT_DIRECTORY "${base_dir}/${binary_subdir}"
        LIBRARY_OUTPUT_DIRECTORY "${base_dir}/${binary_subdir}"
        ARCHIVE_OUTPUT_DIRECTORY "${base_dir}/lib"
        PDB_OUTPUT_DIRECTORY "${base_dir}/bin"
    )
endfunction()

function(clingo_install_target)
    set(targets ${ARGV})
    list(POP_BACK targets install_type)

    if ("${install_type}" STREQUAL "extra" AND CLINGO_INSTALL_EXTRA)
        install(
            TARGETS ${targets}
            EXPORT clingo-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    elseif (("${install_type}" STREQUAL "default" OR "${install_type}" STREQUAL "binary") AND CLINGO_INSTALL_DEFAULT)
        install(
                TARGETS ${targets}
                EXPORT clingo-targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )
    elseif (("${install_type}" STREQUAL "default" OR "${install_type}" STREQUAL "wheel") AND CLINGO_INSTALL_WHEEL)
        install(TARGETS ${targets}
                RUNTIME DESTINATION .
                LIBRARY DESTINATION .
                ARCHIVE DESTINATION .
                INCLUDES DESTINATION .
            )
    endif()
endfunction()
