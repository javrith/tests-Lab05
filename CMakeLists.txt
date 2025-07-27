# Require CMAKE 3.31 or higher
cmake_minimum_required(VERSION 3.31)

# Project name
project(tests)

# Build for C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Any source files in this directory
set(SOURCE_FILES Main.cpp Math.cpp AudioSystem.cpp)

# Name of executable
add_executable(main ${SOURCE_FILES})
