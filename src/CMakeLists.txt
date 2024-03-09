find_package(Threads REQUIRED)
find_package(libfuse REQUIRED)
find_package(Boost REQUIRED)

add_executable(multifs
    app_params.hpp
    file.cpp
    file.hpp
    file_system_interface.hpp
    file_system_noexcept.hpp
    file_system_noexcept_interface.hpp
    file_system_reflector.cpp
    file_system_reflector.hpp
    inode/chmodder.hpp
    inode/chowner.hpp
    inode/fallocater.hpp
    inode/fsyncer.hpp
    inode/link_reader.hpp
    inode/lseeker.hpp
    inode/opener.hpp
    inode/reader.hpp
    inode/releaser.hpp
    inode/truncater.hpp
    inode/unlinker.hpp
    inode/utimenser.hpp
    inode/writer.hpp
    main.cpp
    multi_file_system.cpp
    multi_file_system.hpp
    multifs.cpp
    multifs.hpp
    passthrough_helpers.hpp
    scope_exit.hpp
    symlink.cpp
    symlink.hpp
    thread_safe_access_file_system.hpp
    wrap.hpp
    $<$<CONFIG:Debug>:logged_file_system.hpp>
)

target_link_libraries(multifs PRIVATE
    libfuse::libfuse
    Threads::Threads
    Boost::headers
    ${CMAKE_DL_LIBS})

target_compile_definitions(multifs PRIVATE -DFUSE_USE_VERSION=35)
target_include_directories(multifs PRIVATE ${CMAKE_CURRENT_LIST_DIR})
