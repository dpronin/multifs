cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(multifs
        VERSION 1.0.0
        LANGUAGES CXX C
        DESCRIPTION "Multi File System")

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_subdirectory(src)
