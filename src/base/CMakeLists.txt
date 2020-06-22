set(SERV_BASE_SRCS Buffer.cpp Buffer.h)

add_library(server_base ${SERV_BASE_SRCS})
target_link_libraries(server_base pthread)

install(TARGETS server_base DESTINATION lib)
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/src/base)
