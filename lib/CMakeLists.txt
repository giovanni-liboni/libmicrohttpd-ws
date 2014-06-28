set(CD ${CMAKE_COMMAND} -E chdir)
set(RMDIR ${CMAKE_COMMAND} -E remove_directory)
set(TAR ${CMAKE_COMMAND} -E tar)
set(COPY ${CMAKE_COMMAND} -E copy_directory)
set(RENAME ${CMAKE_COMMAND} -E rename)
set(MOVE ${CMAKE_COMMAND} -E mv)
set(MKDIR ${CMAKE_COMMAND} -E make_directory)

#############################
####### LIBMICROHTTPD #######
#############################

set(microhttpd_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/libmicrohttpd-ws")
set(microhttpd_dir ${CMAKE_CURRENT_BINARY_DIR}/libmicrohttpd-ws)
set(microhttpd_dir ${microhttpd_dir} PARENT_SCOPE)

add_custom_target(microhttpd_install
    COMMAND
      ${RMDIR} ${microhttpd_dir}
    COMMAND
      ${COPY} ${microhttpd_source_dir} ${microhttpd_dir}
    COMMAND
      ${CD} ${microhttpd_dir}
      ./configure --prefix=${microhttpd_dir} --disable-https --disable-curl
    COMMAND
      ${CD} ${microhttpd_dir}
      make -j4
    COMMAND
      ${CD} ${microhttpd_dir}
      make install
)
#############################
####### LIBWEBSOCKETS #######
#############################

set(websockets_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/libwebsockets")
set(websockets_dir ${CMAKE_CURRENT_BINARY_DIR}/libwebsockets)
set(websockets_dir ${websockets_dir} PARENT_SCOPE)

add_custom_target(websockets_install
    COMMAND
      ${MKDIR} ${websockets_dir}
    COMMAND
      ${CD} ${websockets_dir}
      ${CMAKE_COMMAND} ${websockets_source_dir} -DCMAKE_INSTALL_PREFIX:PATH=${websockets_dir}
    COMMAND
      ${CD} ${websockets_dir}
      make -j4
)


add_custom_target( install_libs )
add_dependencies(install_libs
  microhttpd_install
  websockets_install
)
