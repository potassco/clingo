file(GLOB_RECURSE LIBCLASP_SRC src/*.cpp *.h)

include_directories(src .)

add_library(clasp-lib STATIC ${LIBCLASP_SRC})

target_link_libraries(clasp-lib)
set_target_properties(clasp-lib PROPERTIES OUTPUT_NAME clasp)
