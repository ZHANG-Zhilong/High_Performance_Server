cmake_minimum_required(VERSION 3.16)
project(High_Performance_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(
        High_Performance_Server
        server_simple.cpp
        include/this_epoll.h
        include/this_epoll.cpp
        ./include/requestData.h
        ./include/requestData.cpp
        ./include/threadpool.h
        ./include/threadpool.cpp
        include/this_util.h
        include/this_util.cpp
)

add_executable(
        Client
        client/client.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(High_Performance_Server Threads::Threads)