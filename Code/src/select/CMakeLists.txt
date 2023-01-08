cmake_minimum_required(VERSION 3.20)
project(select_prj)


set(SRC_FILES select_demo.cpp)
add_executable(select_demo ${SRC_FILES})

add_executable(select_echo_server select_echo_server.cpp)

#add_executable(client m_thread_client.cpp)
#target_link_libraries(client pthread)

#include(FetchContent)
#FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json)
#FetchContent_MakeAvailable(json)

#FetchContent_Declare(spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz)
#FetchContent_MakeAvailable(spdlog)
#
#target_link_libraries(select_demo PRIVATE spdlog::spdlog_header_only)