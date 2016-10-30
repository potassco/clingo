# [[[cog
#    import os
#    import os.path
#    import re
#    os.chdir(os.path.dirname(cog.inFile))
#    for root, dirnames, filenames in os.walk('.'):
#        for filename in filenames:
#            if re.match(r"^.*\.c$", filename):
#                name = filename[:-2]
#                cog.out("""
#                        add_executable({name} "${{CMAKE_CURRENT_SOURCE_DIR}}/{filname}")
#                        target_link_libraries({name} PRIVATE clingo_lib)
#                        """.format(name=name, filname=filename))
# ]]]

add_executable(version "${CMAKE_CURRENT_SOURCE_DIR}/version.c")
target_link_libraries(version PRIVATE clingo_lib)

add_executable(propagator "${CMAKE_CURRENT_SOURCE_DIR}/propagator.c")
target_link_libraries(propagator PRIVATE clingo_lib)

add_executable(statistics "${CMAKE_CURRENT_SOURCE_DIR}/statistics.c")
target_link_libraries(statistics PRIVATE clingo_lib)

add_executable(solve-async "${CMAKE_CURRENT_SOURCE_DIR}/solve-async.c")
target_link_libraries(solve-async PRIVATE clingo_lib)

add_executable(model "${CMAKE_CURRENT_SOURCE_DIR}/model.c")
target_link_libraries(model PRIVATE clingo_lib)

add_executable(ast "${CMAKE_CURRENT_SOURCE_DIR}/ast.c")
target_link_libraries(ast PRIVATE clingo_lib)

add_executable(symbol "${CMAKE_CURRENT_SOURCE_DIR}/symbol.c")
target_link_libraries(symbol PRIVATE clingo_lib)

add_executable(solve-iteratively "${CMAKE_CURRENT_SOURCE_DIR}/solve-iteratively.c")
target_link_libraries(solve-iteratively PRIVATE clingo_lib)

add_executable(control "${CMAKE_CURRENT_SOURCE_DIR}/control.c")
target_link_libraries(control PRIVATE clingo_lib)

add_executable(backend "${CMAKE_CURRENT_SOURCE_DIR}/backend.c")
target_link_libraries(backend PRIVATE clingo_lib)

add_executable(theory-atoms "${CMAKE_CURRENT_SOURCE_DIR}/theory-atoms.c")
target_link_libraries(theory-atoms PRIVATE clingo_lib)

add_executable(configuration "${CMAKE_CURRENT_SOURCE_DIR}/configuration.c")
target_link_libraries(configuration PRIVATE clingo_lib)

add_executable(symbolic-atoms "${CMAKE_CURRENT_SOURCE_DIR}/symbolic-atoms.c")
target_link_libraries(symbolic-atoms PRIVATE clingo_lib)

# [[[end]]]
