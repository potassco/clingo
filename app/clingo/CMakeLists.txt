file(GLOB_RECURSE CLINGO_SRC *.cpp *.h)

add_library(appclingo-lib STATIC ${CLINGO_SRC})
set_target_properties(appclingo-lib PROPERTIES OUTPUT_NAME appclingo)
