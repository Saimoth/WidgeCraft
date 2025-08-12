cmake_minimum_required(VERSION 3.25)
project(WidgeCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(widgecraft src/main.cpp)
target_include_directories(widgecraft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
