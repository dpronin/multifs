cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(multifs
        VERSION 2.0.1
        LANGUAGES CXX C
        DESCRIPTION "Multi File System")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_subdirectory(src)
