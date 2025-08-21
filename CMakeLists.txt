cmake_minimum_required(VERSION 3.10)
project(CTMPBroadcaster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(ctmp_broadcaster
    src/main.cpp
    src/ctmp_message.cpp
    src/server.cpp
    src/client_handler.cpp
)

target_link_libraries(ctmp_broadcaster PRIVATE pthread)
