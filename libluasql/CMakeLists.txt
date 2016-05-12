file(GLOB_RECURSE LIBLUASQL_SRC *.h)
set(LIBLUASQL_SRC src/luasql.cpp ${CMAKE_CURRENT_BINARY_DIR}/luasql/luasql.h)

foreach(DRIVER SQLITE3 MYSQL ODBC)
	find_package(${DRIVER})
	if(${DRIVER}_FOUND)
		include_directories(${${DRIVER}_INCLUDE_DIR})
		add_definitions(${${DRIVER}_DEFINITIONS})
		string(TOLOWER ${DRIVER} dname)
		set(LIBLUASQL_SRC ${LIBLUASQL_SRC} src/ls_${dname}.cpp)
		set(luasql-libs ${luasql-libs} ${${DRIVER}_LIBRARIES})
		set(WITH_${DRIVER} TRUE)
	endif()
endforeach()

configure_file(luasql/luasql.h.in ${CMAKE_CURRENT_BINARY_DIR}/luasql/luasql.h)

include_directories(src . ${CMAKE_SOURCE_DIR}/liblua ${CMAKE_CURRENT_BINARY_DIR})

add_library(luasql-lib STATIC ${LIBLUASQL_SRC})
target_link_libraries(luasql-lib ${luasql-libs})
set_target_properties(luasql-lib PROPERTIES OUTPUT_NAME luasql)
