cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(multimount VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

find_package(Threads REQUIRED)
set(FUSE_DIR ${CMAKE_CURRENT_LIST_DIR}/fuse-3.10.5)

add_custom_target(libfuse ALL COMMAND meson ${FUSE_DIR}/build ${FUSE_DIR} COMMAND ninja -C ${FUSE_DIR}/build)
add_executable(multifs
    file.cpp
    file.hpp
    file_system_interface.hpp
    file_system_reflector.cpp
    file_system_reflector.hpp
    log.hpp
    multi_file_system.cpp
    multi_file_system.hpp
    multifs.cpp
    passthrough_helpers.hpp
    symlink.cpp
    symlink.hpp
    thread_safe_access_file_system.hpp
)
add_dependencies(multifs libfuse)

target_include_directories(multifs PRIVATE ${FUSE_DIR}/include)
target_link_libraries(multifs PRIVATE
    ${FUSE_DIR}/build/lib/libfuse3.a
    Threads::Threads
    ${CMAKE_DL_LIBS})
# shared_lock does not work with glibc linked statically
# target_link_options(multifs PRIVATE -static)
target_compile_definitions(multifs PRIVATE -DFUSE_USE_VERSION=34)
target_compile_options(multifs PRIVATE PRIVATE "SHELL:-include ${FUSE_DIR}/build/config.h")
