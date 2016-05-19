file(GLOB LIBCLASP_SRC src/*.cpp)
file(GLOB LIBCLASP_INC clasp/*.h)

include_directories(.)
add_library(clasp-lib STATIC ${LIBCLASP_SRC} ${LIBCLASP_INC})
set_target_properties(clasp-lib PROPERTIES OUTPUT_NAME clasp)

#install(FILES ${LIBCLASP_INC} DESTINATION include/clasp)
#install(TARGETS clasp-lib ARCHIVE DESTINATION lib)
