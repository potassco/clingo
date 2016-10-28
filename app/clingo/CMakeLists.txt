set(header_path ${CLINGO_SOURCE_DIR}/app/clingo)
set(source_path ${CLINGO_SOURCE_DIR}/app/clingo)
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
    "${header_path}/src/clingo_app.hh")
set(source
    "${source_path}/src/clingo_app.cc"
    "${source_path}/src/main.cc")
# [[[end]]]

list(APPEND header "${CLASP_SOURCE_DIR}/app/clasp_app.h")
list(APPEND source "${CLASP_SOURCE_DIR}/app/clasp_app.cpp")

set(ide_header_group "Header Files")
source_group("${ide_header_group}" FILES ${header})

add_executable(clingo ${header} ${source})
target_include_directories(clingo PRIVATE "${CLASP_SOURCE_DIR}/app")
target_link_libraries(clingo clingo_build)
