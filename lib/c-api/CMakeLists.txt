# [[[header: include
set(ide_header_group "Header Files")
set(header-group
    "include/clingo.h")
source_group("${ide_header_group}" FILES ${header-group})
set(header-group-clingo
    "include/clingo/app.h"
    "include/clingo/ast.h"
    "include/clingo/backend.h"
    "include/clingo/base.h"
    "include/clingo/config.h"
    "include/clingo/control.h"
    "include/clingo/core.h"
    "include/clingo/model.h"
    "include/clingo/observe.h"
    "include/clingo/propagate.h"
    "include/clingo/script.h"
    "include/clingo/shared.h"
    "include/clingo/solve.h"
    "include/clingo/stats.h"
    "include/clingo/symbol.h"
    "include/clingo/theory.h")
source_group("${ide_header_group}\\clingo" FILES ${header-group-clingo})
set(header
    ${header-group}
    ${header-group-clingo})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "src/ast.cc"
    "src/ast.hh"
    "src/ast_yaml.cc"
    "src/backend.cc"
    "src/base.cc"
    "src/config.cc"
    "src/control.cc"
    "src/control.hh"
    "src/core.cc"
    "src/core.hh"
    "src/lib.cc"
    "src/lib.hh"
    "src/main.cc"
    "src/model.cc"
    "src/observe.cc"
    "src/opts.hh"
    "src/propagate.cc"
    "src/script.cc"
    "src/solve.cc"
    "src/stats.cc"
    "src/symbol.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (CLINGO_BUILD_SHARED)
    set(libtype SHARED)
endif()

add_library(clingo ${libtype})
target_sources(clingo PRIVATE ${header} ${source})
clingo_target_properties(clingo "lib")
target_include_directories(clingo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(clingo PRIVATE clingo::control)
string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if(build_type STREQUAL "debug")
    target_compile_definitions(clingo PRIVATE CLINGO_DEBUG)
endif()
if (CLINGO_USE_VISIBILITY)
    target_compile_definitions(clingo PRIVATE $<BUILD_INTERFACE:CLINGO_BUILD_LIBRARY>)
else()
    target_compile_definitions(clingo PUBLIC CLINGO_NO_VISIBILITY)
endif()
if (CLINGO_USE_VERSION_SCRIPT AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set_target_properties(clingo PROPERTIES LINK_FLAGS "-Wl,--version-script='${CMAKE_SOURCE_DIR}/cmake/clingo.map'")
endif()
if (CLINGO_INSTALL_DEFAULT)
    clingo_install_target(clingo default)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
elseif (CLINGO_INSTALL_WHEEL AND CLINGO_BUILD_SHARED)
    install(DIRECTORY include/ DESTINATION clingo-include FILES_MATCHING PATTERN "*.h")
    install(
        TARGETS clingo
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION ".")
endif()
