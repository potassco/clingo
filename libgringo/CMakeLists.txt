file(GLOB_RECURSE LIBGRINGO_SRC    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp *.h)
file(GLOB_RECURSE LIBGRINGO_LEXER  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.r2c)
file(GLOB_RECURSE LIBGRINGO_PARSER RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.y)

include_directories(src .)

RE2C(LIBGRINGO_SRC ${LIBGRINGO_LEXER})
LEMON(LIBGRINGO_SRC ${LIBGRINGO_PARSER})

add_library(gringo-lib STATIC ${LIBGRINGO_SRC})

target_link_libraries(gringo-lib)
set_target_properties(gringo-lib PROPERTIES OUTPUT_NAME gringo)

if(PCHSupport_FOUND)
	ADD_PRECOMPILED_HEADER(gringo-lib ${CMAKE_SOURCE_DIR}/libgringo/gringo/gringo.h)
endif()
