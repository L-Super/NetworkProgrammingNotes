cmake_minimum_required(VERSION 3.20)
project(NetWorkCodeDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(epoll)
add_subdirectory(select)
add_subdirectory(pingpong)
add_executable(client client.cpp)
