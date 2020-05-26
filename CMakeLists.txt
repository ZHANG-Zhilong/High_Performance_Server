cmake_minimum_required(VERSION 3.16)
project(High_Performance_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(
        High_Performance_Server
        server_simple.cpp
        src/this_epoll.h
        src/this_epoll.cpp
        src/requestData.h
        src/requestData.cpp
        src/threadpool.h
        src/threadpool.cpp
        src/this_util.h
        src/this_util.cpp
)

add_executable(
        Client
        client/client.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(High_Performance_Server Threads::Threads)