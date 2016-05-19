if(${GRINGO_TYPE} STREQUAL "gringo")
	file(GLOB GRINGO_INC *.h)
	set (GRINGO_SRC main.cpp gringo_app.cpp gringo_options.cpp)
        include_directories (${CMAKE_SOURCE_DIR}/libgringo ${CMAKE_SOURCE_DIR}/libprogram_opts)
else()
	file(GLOB GRINGO_INC *.h clasp/*.h)
	file(GLOB GRINGO_SRC *.cpp clasp/*.cpp)
        include_directories (${CMAKE_SOURCE_DIR}/libgringo ${CMAKE_SOURCE_DIR}/libprogram_opts ${CMAKE_SOURCE_DIR}/libclasp)
endif()

add_executable (gringo-app ${GRINGO_SRC} ${GRINGO_INC})

if(${GRINGO_TYPE} STREQUAL "iclingo")
	target_link_libraries (gringo-app gringo-lib clasp-lib program_opts-lib)
	set_target_properties(gringo-app PROPERTIES OUTPUT_NAME iclingo)
endif()

if(${GRINGO_TYPE} STREQUAL "clingo")
	target_link_libraries (gringo-app gringo-lib clasp-lib program_opts-lib)
	set_target_properties(gringo-app PROPERTIES OUTPUT_NAME clingo)
endif()

if(${GRINGO_TYPE} STREQUAL "gringo")
	target_link_libraries (gringo-app gringo-lib program_opts-lib)
	set_target_properties(gringo-app PROPERTIES OUTPUT_NAME gringo)
endif()

install(TARGETS gringo-app RUNTIME DESTINATION bin)
