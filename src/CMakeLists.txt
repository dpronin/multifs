find_package(Threads REQUIRED)
find_package(libfuse REQUIRED)
find_package(Boost REQUIRED)

add_executable(multifs
    factory_interface.hpp
    factory_shared_interface.hpp
    factory_unique_interface.hpp
    file.cpp
    file.hpp
    file_system_interface.hpp
    file_system_noexcept.hpp
    file_system_noexcept_interface.hpp
    file_system_reflector.cpp
    file_system_reflector.hpp
    fs_factory_interface.hpp
    fs_reflector_factory.hpp
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
    logged_file_system.hpp
    main.cpp
    multi_file_system.cpp
    multi_file_system.hpp
    multi_fs_factory.hpp
    multifs.cpp
    multifs.hpp
    passthrough_helpers.hpp
    rvwrap.hpp
    scope_exit.hpp
    symlink.cpp
    symlink.hpp
    thread_safe_access_file_system.hpp
    wrap.hpp
)

target_link_libraries(multifs PRIVATE
    libfuse::libfuse
    Threads::Threads
    Boost::headers
    ${CMAKE_DL_LIBS})

target_compile_definitions(multifs PRIVATE -DFUSE_USE_VERSION=34)
target_include_directories(multifs PRIVATE ${CMAKE_CURRENT_LIST_DIR})
