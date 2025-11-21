# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(clingo-app)
target_sources(clingo-app PRIVATE ${source})
clingo_target_properties(TARGETS clingo-app FOLDER exe TYPE default)
# NOTE: we set the archive output directory to avoid conflicts with the generated implib
set_target_properties(clingo-app PROPERTIES OUTPUT_NAME clingo ARCHIVE_OUTPUT_DIRECTORY .)
target_include_directories(clingo-app PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(clingo-app PRIVATE clingo clingo-python-embed)
set_target_properties(clingo-app PROPERTIES IMPORTED_IMPLIB clingo-app.lib)