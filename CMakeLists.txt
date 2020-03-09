cmake_minimum_required(VERSION 3.15)
project(single_httpdemo)

file(GLOB SOURCES "main/*.cpp")
file(GLOB HEADERS "main/*.h")

set(CMAKE_CXX_STANDARD 11)

add_executable(single_httpdemo main.cpp ${SOURCES} ${HEADERS})
link_directories(${PROJECT_SOURCE_DIR}/libs) #添加动态连接库的路径
target_link_libraries(single_httpdemo -lws2_32 ) #添加ws2_32.dll