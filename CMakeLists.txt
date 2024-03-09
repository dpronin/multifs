cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(multifs
        VERSION 3.0.0
        LANGUAGES CXX C
        HOMEPAGE_URL "https://github.com/dpronin/multifs"
        DESCRIPTION "multifs (Multi File System) supports aggregating multiple underlying file systems into one giving an opportunity for a user to store huge files if none of existing file systems is able to accommodate such files due to lack of storage. multifs slices huge files into chunks and places these chunks within underlying file systems, but a user works with multifs as with the file system with contiguous file space")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_subdirectory(src)
