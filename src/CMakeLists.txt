add_subdirectory(example)

set(COPY ${CMAKE_COMMAND} -E copy_directory)

add_custom_target(run_main
	COMMAND
	${COPY} "${CMAKE_CURRENT_SOURCE_DIR}/example/www" "${CMAKE_CURRENT_BINARY_DIR}/example/www"
	
	COMMAND main 9090
)
