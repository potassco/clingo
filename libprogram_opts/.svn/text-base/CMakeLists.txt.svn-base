file(GLOB LIBPROGRAM_OPTS_SRC src/*.cpp)
file(GLOB LIBPROGRAM_OPTS_INC program_opts/*.h)

include_directories(.)

add_library(program_opts-lib STATIC ${LIBPROGRAM_OPTS_SRC} ${LIBPROGRAM_OPTS_INC})
set_target_properties(program_opts-lib PROPERTIES OUTPUT_NAME libprogram_opts)
