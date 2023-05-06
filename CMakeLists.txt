cmake_minimum_required(VERSION 3.25)
project(ssh_tunnel)

set(CMAKE_CXX_STANDARD 17)

add_executable(ssh_tunnel main.cpp Config.cpp Config.h TunnelManager.cpp TunnelManager.h)
target_link_libraries(${PROJECT_NAME} jsoncpp)