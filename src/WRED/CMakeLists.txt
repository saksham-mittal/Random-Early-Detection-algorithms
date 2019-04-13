cmake_minimum_required(VERSION 3.7.2)
project(Random_Early_Detection_Algorithms)
find_package (Threads)
#Bring the headers, such as Student.h into the project
include_directories(include)

set(EXECUTABLE_OUTPUT_PATH ../../bin/WRED)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/WRED/*.cpp")

add_executable(client ../../src/WRED/client.cpp)
add_executable(server ../../src/WRED/server.cpp)
add_executable(gateway ../../src/WRED/gateway.cpp)
add_executable(testCases ../../src/Test.cpp)

target_link_libraries (client ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (gateway ${CMAKE_THREAD_LIBS_INIT})