cmake_minimum_required(VERSION 3.16)
project(High_Performance_Server)

set(CMAKE_CXX_STANDARD 14)

add_executable(
	High_Performance_Server 
	server.cpp 
	include/wrap.cpp 
	include/wrap.h 
	include/epoll_tool.cpp 
	include/epoll_tool.h
)
