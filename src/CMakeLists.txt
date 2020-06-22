set (server_server_SRCS this_epoll.cpp this_epoll.h requestData.cpp requestData.h threadpool.cpp threadpool.h this_util.cpp this_util.h)

add_library(server_server ${server_server_SRCS})
target_link_libraries(server_server pthread)

install (TARGETS server_server DESTINATION lib)
file(GLOB HEADERS "*.h")
install (FILES ${HEADERS} DESTINATION include/src)

