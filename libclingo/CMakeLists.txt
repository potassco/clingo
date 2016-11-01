set(header_path ${CLINGO_SOURCE_DIR}/libclingo)
set(source_path ${CLINGO_SOURCE_DIR}/libclingo)
# [[[cog
#    import os
#    import os.path
#    import re
#    os.chdir(os.path.dirname(cog.inFile))
#    header, source = "set(header\n", "set(source\n"
#    for root, dirnames, filenames in os.walk('.'):
#        if "tests" in dirnames:
#            dirnames.remove("tests")
#        for filename in filenames:
#            if re.match(r"^.*\.(h|hh|hpp)$", filename):
#                header+= "    \"${{header_path}}{src}\"\n".format(src=os.path.join(root, filename)[1:])
#            if re.match(r"^.*\.(c|cc|cpp)$", filename):
#                source+= "    \"${{source_path}}{src}\"\n".format(src=os.path.join(root, filename)[1:])
#    cog.out(header[:-1] + ")\n")
#    cog.out(source[:-1] + ")\n")
# ]]]
set(header
    "${header_path}/clingo/clingocontrol.hh"
    "${header_path}/clingo/clingo_app.hh")
set(source
    "${source_path}/src/clingo_app.cc"
    "${source_path}/src/clingocontrol.cc")
# [[[end]]]

list(APPEND header "${CLASP_SOURCE_DIR}/app/clasp_app.h")
list(APPEND source "${CLASP_SOURCE_DIR}/app/clasp_app.cpp")

set(ide_header_group "Header Files")
source_group("${ide_header_group}" FILES ${header})

if (BUILD_CLINGO_LIB_SHARED)
    set(clingo_lib_type SHARED)
else()
    set(clingo_lib_type STATIC)
endif()

add_library(libclingo ${clingo_lib_type} ${clingo_lib_type} ${header} ${source})
target_include_directories(libclingo PRIVATE
    "${CLASP_SOURCE_DIR}/app"
    "$<BUILD_INTERFACE:${header_path}>")
target_include_directories(libclingo PUBLIC
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libgringo>")
target_link_libraries(libclingo PRIVATE libgringo clasp)
target_compile_definitions(libclingo PRIVATE CLINGO_BUILD_LIBRARY)
set_target_properties(libclingo PROPERTIES OUTPUT_NAME clingo)

if (CLINGO_ENABLE_LUA) 
    target_compile_definitions(libclingo PRIVATE WITH_LUA)
    target_include_directories(libclingo PRIVATE ${LUA_INCLUDE_DIR})
endif()
if (CLINGO_ENABLE_PYTHON) 
    target_compile_definitions(libclingo PRIVATE WITH_PYTHON)
    target_include_directories(libclingo PRIVATE ${PYTHON_INCLUDE_DIRS})
endif()

if (BUILD_CLINGO_LIB_SHARED)
    set(clingo_lib_type SHARED)
    foreach(target libclingo libgringo potassco clasp libreify)
        if (POLICY CMP0063)
            set_target_properties(${target} PROPERTIES CXX_VISIBILITY_PRESET hidden)
        endif()
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE on)
    endforeach()
endif()
