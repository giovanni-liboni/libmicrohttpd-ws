project("websockets")
cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "-Wall -Werror")

add_subdirectory(lib)

include_directories(
  ${microhttpd_dir}/include
)
link_directories(
  ${microhttpd_dir}/lib
)

add_subdirectory(src)
