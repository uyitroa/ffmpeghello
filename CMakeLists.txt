cmake_minimum_required(VERSION 3.12)
project(ffmpeghello)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
link_directories(lib)
add_executable(ffmpeghello test.cpp FrameWriter.cpp FrameWriter.h)
target_link_libraries(ffmpeghello avcodec avutil avformat swresample swscale)