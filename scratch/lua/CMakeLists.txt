cmake_minimum_required (VERSION 2.6)
project (lua) # project here actually means solution in premake

file(GLOB SOURCES src/*.c)

message ("sources: ${SOURCES}")

add_library (lualib STATIC ${SOURCES})
set_target_properties(lualib PROPERTIES OUTPUT_NAME "lua")

install(TARGETS lualib ARCHIVE DESTINATION lib)
install(FILES src/lua.h src/luaconf.h src/lualib.h src/lauxlib.h src/lua.hpp DESTINATION include)
