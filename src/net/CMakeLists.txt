set (SERV_NET_SRCS SocketOpts.cpp SocketOpts.h EventLoop.cpp EventLoop.h)

add_library(server_net ${SERV_NET_SRCS})
target_link_libraries( server_net pthread)

install (TARGETS server_net DESTINATION lib)
file(GLOB HEADERS "*.h")
install (FILES ${HEADERS} DESTINATION include/src/net)