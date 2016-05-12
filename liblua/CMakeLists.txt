file(GLOB_RECURSE LIBLUA_SRC src/*.cpp *.h)

include_directories(src .)

add_library(lua-lib STATIC ${LIBLUA_SRC})

target_link_libraries(lua-lib)
set_target_properties(lua-lib PROPERTIES OUTPUT_NAME lua)
