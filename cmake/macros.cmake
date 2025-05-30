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

function(clingo_target_properties)
    set(options)
    set(single_values FOLDER TYPE SUBDIR)
    set(multi_values TARGETS)
    cmake_parse_arguments(clingo "${options}" "${single_values}" "${multi_values}" ${ARGV})

    set(binary_subdir "bin")
    set(library_subdir "lib")
    if(clingo_SUBDIR)
        set(binary_subdir "bin/${clingo_SUBDIR}")
        set(library_subdir "lib/${clingo_SUBDIR}")
    endif()

    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(binary_subdir "${binary_subdir}/$<CONFIG>")
        set(library_subdir "${library_subdir}/$<CONFIG>")
    endif()

    if (clingo_FOLDER)
        set_target_properties(${clingo_TARGETS} PROPERTIES
        FOLDER "${clingo_FOLDER}"
        POSITION_INDEPENDENT_CODE ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${binary_subdir}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${binary_subdir}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${library_subdir}"
        PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${binary_subdir}")
    endif()

    if(clingo_TYPE STREQUAL "extra" AND CLINGO_INSTALL_EXTRA)
        install(
            TARGETS ${clingo_TARGETS}
            EXPORT clingo-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    elseif(clingo_TYPE STREQUAL "default" AND CLINGO_INSTALL_DEFAULT)
        install(
            TARGETS ${clingo_TARGETS}
            EXPORT clingo-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()
endfunction()
