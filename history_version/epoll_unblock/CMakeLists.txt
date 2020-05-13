cmake_minimum_required(VERSION 3.16)
project(High_Performance_Server)

set(CMAKE_CXX_STANDARD 14)

add_executable(High_Performance_Server server.cpp wrap.cpp wrap.h)