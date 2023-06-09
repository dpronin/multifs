find_package(Threads REQUIRED)
find_package(libfuse REQUIRED)

add_executable(multifs
    file.cpp
    file.hpp
    file_system_interface.hpp
    file_system_reflector.cpp
    file_system_reflector.hpp
    inode/inode_chmodder.hpp
    inode/inode_chowner.hpp
    inode/inode_fallocater.hpp
    inode/inode_fsyncer.hpp
    inode/inode_link_reader.hpp
    inode/inode_lseeker.hpp
    inode/inode_opener.hpp
    inode/inode_reader.hpp
    inode/inode_releaser.hpp
    inode/inode_truncater.hpp
    inode/inode_unlinker.hpp
    inode/inode_utimenser.hpp
    inode/inode_writer.hpp
    log.hpp
    multi_file_system.cpp
    multi_file_system.hpp
    multifs.cpp
    passthrough_helpers.hpp
    symlink.cpp
    symlink.hpp
    thread_safe_access_file_system.hpp
)

target_link_libraries(multifs PRIVATE
    libfuse::libfuse
    Threads::Threads
    ${CMAKE_DL_LIBS})
# shared_lock does not work with glibc linked statically
# target_link_options(multifs PRIVATE -static)
target_compile_definitions(multifs PRIVATE -DFUSE_USE_VERSION=34)
target_include_directories(multifs PRIVATE ${CMAKE_CURRENT_LIST_DIR})
