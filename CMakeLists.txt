cmake_minimum_required(VERSION 3.6)
project(tinyhttpd)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        httpd.c)

add_executable(tinyhttpd ${SOURCE_FILES})