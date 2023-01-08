cmake_minimum_required(VERSION 3.23)
project(Rect)

file(GLOB_RECURSE PROJECT_SRC cpp/*.cpp cpp/*.c)
include_directories(include)

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)