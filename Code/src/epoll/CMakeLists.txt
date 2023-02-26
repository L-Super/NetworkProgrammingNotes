cmake_minimum_required(VERSION 3.20)
project(epoll_demo)


set(SRC_FILES epoll_lt_echo_server.cpp)
add_executable(epoll_lt_server ${SRC_FILES})
add_executable(epoll_et_server epoll_et_echo_server.cpp)

#include(FetchContent)
#FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json)
#FetchContent_MakeAvailable(json)

#FetchContent_Declare(spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz)
#FetchContent_MakeAvailable(spdlog)

#target_link_libraries(epoll_demo PRIVATE spdlog::spdlog_header_only)