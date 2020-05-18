cmake_minimum_required(VERSION 3.16)
project(High_Performance_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(
        High_Performance_Server
        src/server_simple.cpp
        include/thread_pool.cpp
        include/thread_pool.h
        include/socket_tool.cpp
        include/socket_tool.h
        include/epoll_tool.cpp
        include/epoll_tool.h
        include/util.cpp
        include/util.h
        include/request_data.cpp
        include/request_data.h
)

add_executable(
        Client
        client/client.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(High_Performance_Server Threads::Threads)