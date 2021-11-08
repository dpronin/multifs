cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(multifs VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(FUSE_DIR ${CMAKE_CURRENT_LIST_DIR}/fuse-3.10.5)
set(FUSE_INCLUDE_DIR ${FUSE_DIR}/include)
set(FUSE_LIB_DIR ${FUSE_DIR}/build/lib/)

add_custom_target(libfuse ALL COMMAND meson ${FUSE_DIR}/build ${FUSE_DIR} COMMAND ninja -C ${FUSE_DIR}/build)
add_subdirectory(src)
