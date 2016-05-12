file(GLOB_RECURSE LIBPROGRAM_OPTS_SRC src/*.cpp *.h)

include_directories(src .)

add_library(program_opts-lib STATIC ${LIBPROGRAM_OPTS_SRC})

target_link_libraries(program_opts-lib)
set_target_properties(program_opts-lib PROPERTIES OUTPUT_NAME program_opts)

