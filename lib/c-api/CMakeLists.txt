if (PARSER_BUILD_SHARED)
    set(libtype SHARED)
endif()

add_library(clingo ${libtype}
    "src/ast.cc"
    "src/ast_yaml.cc"
    "src/backend.cc"
    "src/base.cc"
    "src/config.cc"
    "src/control.cc"
    "src/core.cc"
    "src/lib.cc"
    "src/main.cc"
    "src/model.cc"
    "src/observe.cc"
    "src/propagate.cc"
    "src/script.cc"
    "src/solve.cc"
    "src/stats.cc"
    "src/symbol.cc"
    )
clingo_target_properties(clingo "lib/clingo")
target_include_directories(clingo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(clingo PRIVATE clingo::control)
string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if(build_type STREQUAL "debug")
    target_compile_definitions(clingo PRIVATE CLINGO_DEBUG)
endif()
if (PARSER_USE_VISIBILITY)
    target_compile_definitions(clingo PUBLIC CLINGO_BUILD_LIBRARY)
else()
    target_compile_definitions(clingo PUBLIC CLINGO_NO_VISIBILITY)
endif()
if (PARSER_USE_VERSION_SCRIPT AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set_target_properties(clingo PROPERTIES LINK_FLAGS "-Wl,--version-script='${CMAKE_SOURCE_DIR}/cmake/clingo.map'")
endif()
