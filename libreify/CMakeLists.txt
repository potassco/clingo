set(header_path ${CLINGO_SOURCE_DIR}/libreify)
set(source_path ${CLINGO_SOURCE_DIR}/libreify)
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
    "${header_path}/reify/program.hh"
    "${header_path}/reify/util.hh")
set(source
    "${source_path}/src/program.cc")
# [[[end]]]

set(ide_header_group "Header Files")
source_group("${ide_header_group}" FILES ${header})
add_library(libreify ${header} ${source})
target_include_directories(libreify PUBLIC
    "$<BUILD_INTERFACE:${header_path}>"
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libgringo>")
target_link_libraries(libreify PUBLIC potassco)
set_target_properties(libreify PROPERTIES OUTPUT_NAME reify)
