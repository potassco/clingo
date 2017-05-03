add_executable(lpconvert lpconvert.cpp)
target_link_libraries(lpconvert libpotassco)
set_target_properties(lpconvert PROPERTIES FOLDER exe)

install(TARGETS lpconvert EXPORT lpconvert DESTINATION "bin")
