set(files
	clasp_app.cpp
	clasp_app.h
	main.cpp)
add_executable(clasp ${files})
set_target_properties(clasp PROPERTIES FOLDER exe)
if (CLASP_BUILD_STATIC AND UNIX AND NOT APPLE)
	if (CLASP_BUILD_WITH_THREADS)
		string(CONCAT refs "-Wl,-u,pthread_cancel,-u,pthread_cond_broadcast,"
		                   "-u,pthread_cond_destroy,-u,pthread_cond_signal,"
		                   "-u,pthread_cond_timedwait,-u,pthread_cond_wait,"
		                   "-u,pthread_create,-u,pthread_detach,-u,pthread_join,"
		                   "-u,pthread_equal")
		target_link_libraries(clasp ${refs})
	endif()
	target_link_libraries(clasp "-static")
endif()
target_link_libraries(clasp libclasp)

install(TARGETS clasp EXPORT clasp DESTINATION "bin")
