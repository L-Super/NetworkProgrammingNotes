cmake_minimum_required(VERSION 3.20)
project(epoll_demo)


set(SERVER_SRC_FILES ping_server.cpp)
add_executable(ping_server ${SERVER_SRC_FILES})

set(CLIENT_SRC_FILES ping_client.cpp)
add_executable(ping_client ${CLIENT_SRC_FILES})