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

add_library(clingo_lib ${clingo_lib_type} ${clingo_lib_type} ${header} ${source})
target_include_directories(clingo_lib PRIVATE
    "${CLASP_SOURCE_DIR}/app"
    "$<BUILD_INTERFACE:${header_path}>")
target_include_directories(clingo_lib PUBLIC
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libgringo>")
target_link_libraries(clingo_lib PRIVATE gringo_build clasp)
set_target_properties(clingo_lib PROPERTIES OUTPUT_NAME clingo)

if (BUILD_CLINGO_LIB_SHARED)
    set(clingo_lib_type SHARED)
    foreach(target clingo_lib gringo_build potassco clasp reify_build)
        if (POLICY CMP0063)
            set_target_properties(${target} PROPERTIES CXX_VISIBILITY_PRESET hidden)
        endif()
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE on)
    endforeach()
endif()
