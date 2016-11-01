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
#                        target_link_libraries({name} PRIVATE libclingo)
#                        """.format(name=name, filname=filename))
# ]]]

add_executable(version "${CMAKE_CURRENT_SOURCE_DIR}/version.c")
target_link_libraries(version PRIVATE libclingo)

add_executable(propagator "${CMAKE_CURRENT_SOURCE_DIR}/propagator.c")
target_link_libraries(propagator PRIVATE libclingo)

add_executable(statistics "${CMAKE_CURRENT_SOURCE_DIR}/statistics.c")
target_link_libraries(statistics PRIVATE libclingo)

add_executable(solve-async "${CMAKE_CURRENT_SOURCE_DIR}/solve-async.c")
target_link_libraries(solve-async PRIVATE libclingo)

add_executable(model "${CMAKE_CURRENT_SOURCE_DIR}/model.c")
target_link_libraries(model PRIVATE libclingo)

add_executable(ast "${CMAKE_CURRENT_SOURCE_DIR}/ast.c")
target_link_libraries(ast PRIVATE libclingo)

add_executable(symbol "${CMAKE_CURRENT_SOURCE_DIR}/symbol.c")
target_link_libraries(symbol PRIVATE libclingo)

add_executable(solve-iteratively "${CMAKE_CURRENT_SOURCE_DIR}/solve-iteratively.c")
target_link_libraries(solve-iteratively PRIVATE libclingo)

add_executable(control "${CMAKE_CURRENT_SOURCE_DIR}/control.c")
target_link_libraries(control PRIVATE libclingo)

add_executable(backend "${CMAKE_CURRENT_SOURCE_DIR}/backend.c")
target_link_libraries(backend PRIVATE libclingo)

add_executable(theory-atoms "${CMAKE_CURRENT_SOURCE_DIR}/theory-atoms.c")
target_link_libraries(theory-atoms PRIVATE libclingo)

add_executable(configuration "${CMAKE_CURRENT_SOURCE_DIR}/configuration.c")
target_link_libraries(configuration PRIVATE libclingo)

add_executable(symbolic-atoms "${CMAKE_CURRENT_SOURCE_DIR}/symbolic-atoms.c")
target_link_libraries(symbolic-atoms PRIVATE libclingo)

# [[[end]]]
