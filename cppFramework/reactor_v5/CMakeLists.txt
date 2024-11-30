cmake_minimum_required(VERSION 3.0)

project(Reactor_singleProcess_multitThreading)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -lpthread")

aux_source_directory(src SRC_LIST)

include_directories(include)

add_executable(reactor_v5  ${SRC_LIST})
