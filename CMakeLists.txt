cmake_minimum_required(VERSION 3.25)
project(ssh_tunnel)

set(CMAKE_CXX_STANDARD 17)

add_executable(ssh_tunnel main.cpp Config.cpp Config.h TunnelManager.cpp TunnelManager.h TablePrinter.cpp)
target_link_libraries(${PROJECT_NAME} jsoncpp)
add_library(bprinter STATIC
        bprinter/table_printer.h
        bprinter/impl/table_printer.tpp.h
        TablePrinter.cpp
)
