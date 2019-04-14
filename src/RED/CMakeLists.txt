cmake_minimum_required(VERSION 3.7.2)
project(Random_Early_Detection_Algorithms)
find_package (Threads)

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH ../../bin/RED)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/RED/*.cpp")

add_executable(client ../../src/RED/client.cpp)
add_executable(server ../../src/RED/server.cpp)
add_executable(gateway ../../src/RED/gateway.cpp)
add_executable(testCases ../../src/Test.cpp)

target_link_libraries (client ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (gateway ${CMAKE_THREAD_LIBS_INIT})