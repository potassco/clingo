MACRO(LEMON VAR)
    FOREACH(SRC ${ARGN})
        GET_FILENAME_COMPONENT(DST  "${SRC}" PATH)
        GET_FILENAME_COMPONENT(NAME "${SRC}" NAME_WE)
        IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DST}/${NAME}.cpp" AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DST}/${NAME}.h")
            GET_TARGET_PROPERTY(lemon_exists lemon TYPE)
            IF(NOT lemon_exists)
                IF(CMAKE_CROSSCOMPILING)
                    # for some reason this has to be done directly in front of the custom command
                    SET(IMPORT_LEMON "IMPORTFILE-NOTFOUND" CACHE FILEPATH "Path to the export file of the native lemon build")
                    INCLUDE(${IMPORT_LEMON})
                ELSE()
                    add_subdirectory("${CMAKE_SOURCE_DIR}/lemon" "${CMAKE_BINARY_DIR}/lemon")
                ENDIF()
            ENDIF()
            IF(UNIX)
                SET(COPY_OR_LINK create_symlink)
            ELSE()
                SET(COPY_OR_LINK copy_if_different)
            ENDIF()
            ADD_CUSTOM_COMMAND(
                OUTPUT "${DST}/lempar.c"
                COMMAND cmake -E make_directory "${DST}"
                COMMAND cmake -E ${COPY_OR_LINK} "${CMAKE_SOURCE_DIR}/lemon/lempar.c" "${DST}/lempar.c"
                MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/lemon/lempar.c"
            )
            ADD_CUSTOM_COMMAND(
                OUTPUT "${DST}/${NAME}.cpp" "${DST}/${NAME}.h"
                COMMAND cmake -E ${COPY_OR_LINK} "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}" "${NAME}.y"
                COMMAND lemon -q "${NAME}.y"
                COMMAND cmake -E ${COPY_OR_LINK} ${NAME}.c ${NAME}.cpp
                MAIN_DEPENDENCY "${SRC}"
                DEPENDS lemon "${CMAKE_CURRENT_BINARY_DIR}/${DST}/lempar.c"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${DST}"
            )
            SET(${VAR} ${${VAR}} "${DST}/${NAME}.cpp" "${DST}/${NAME}.h")
            INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/${DST}")
        ENDIF()
    ENDFOREACH()
ENDMACRO()
