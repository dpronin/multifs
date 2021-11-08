find_package(Threads REQUIRED)

add_executable(multifs
    file.cpp
    file.hpp
    file_system_interface.hpp
    file_system_reflector.cpp
    file_system_reflector.hpp
    inode_chmodder.hpp
    inode_chowner.hpp
    inode_fallocater.hpp
    inode_fsyncer.hpp
    inode_link_reader.hpp
    inode_lseeker.hpp
    inode_opener.hpp
    inode_reader.hpp
    inode_releaser.hpp
    inode_truncater.hpp
    inode_unlinker.hpp
    inode_utimenser.hpp
    inode_writer.hpp
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

target_include_directories(multifs PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(multifs PRIVATE
    ${FUSE_LIB_DIR}/libfuse3.a
    Threads::Threads
    ${CMAKE_DL_LIBS})
# shared_lock does not work with glibc linked statically
# target_link_options(multifs PRIVATE -static)
target_compile_definitions(multifs PRIVATE -DFUSE_USE_VERSION=34)
target_compile_options(multifs PRIVATE PRIVATE "SHELL:-include ${FUSE_DIR}/build/config.h")
