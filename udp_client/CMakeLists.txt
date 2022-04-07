# Minimum CMake required
cmake_minimum_required(VERSION 2.8)

# Project
project(Client)

# Find required protobuf package
find_package(Protobuf REQUIRED)
if (PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
else ()
    message(FATAL_ERROR "protobuf library is needed but cant be found")
endif ()

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS addressbook.proto)
message(${PROTO_SRCS} ${PROTO_HDRS})
add_executable(Client Client.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(Client ${PROTOBUF_LIBRARIES})
target_link_libraries(Client lz4)
