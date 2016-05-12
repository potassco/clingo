MACRO(RE2C VAR)
    FOREACH(SRC ${ARGN})
        GET_FILENAME_COMPONENT(DST "${SRC}" PATH)
        GET_FILENAME_COMPONENT(NAME "${SRC}" NAME_WE)
        IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DST}/${NAME}.cpp")
            IF(NOT RE2C_EXECUTABLE)
                FIND_PACKAGE(RE2C REQUIRED)
            ENDIF()
            ADD_CUSTOM_COMMAND(
                OUTPUT "${DST}/${NAME}.cpp"
                COMMAND cmake -E make_directory "${DST}"
                COMMAND "${RE2C_EXECUTABLE}" -o "${DST}/${NAME}.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}"
                MAIN_DEPENDENCY "${SRC}"
            )
            SET(${VAR} ${${VAR}} "${CMAKE_CURRENT_BINARY_DIR}/${DST}/${NAME}.cpp")
        ENDIF()
    ENDFOREACH()
ENDMACRO()
