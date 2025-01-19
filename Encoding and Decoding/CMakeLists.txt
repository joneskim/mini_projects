cmake_minimum_required(VERSION 3.10)

# Set the project name
project(EncodingAndDecoding)

# Use C++20 standard
set(CMAKE_CXX_STANDARD 20)

# Add the executable
add_executable(asciitable asciitable.cpp)
add_executable(asciiart asciiart.cpp)
