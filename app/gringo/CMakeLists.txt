file(GLOB_RECURSE GRINGO_SRC *.cpp *.h)

add_library(appgringo-lib STATIC ${GRINGO_SRC})
set_target_properties(appgringo-lib PROPERTIES OUTPUT_NAME appgringo)
