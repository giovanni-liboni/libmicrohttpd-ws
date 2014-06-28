set(CD ${CMAKE_COMMAND} -E chdir)
set(RMDIR ${CMAKE_COMMAND} -E remove_directory)
set(TAR ${CMAKE_COMMAND} -E tar)
set(COPY ${CMAKE_COMMAND} -E copy_directory)
set(RENAME ${CMAKE_COMMAND} -E rename)
set(MOVE ${CMAKE_COMMAND} -E mv)

set(microhttpd_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/libmicrohttpd-ws")
