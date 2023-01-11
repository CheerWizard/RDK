cmake_minimum_required(VERSION 3.23)
project(Rect)

file(GLOB_RECURSE PROJECT_SRC cpp/*.cpp cpp/*.c)

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

# includes
include_directories(include)
# GLFW
add_subdirectory(vendor/glfw)
include_directories(vendor/glfw/include)
# GLM
add_subdirectory(vendor/glm)
include_directories(vendor/glm/glm)
# Vulkan
include_directories(vendor/vulkan/Include)

target_link_libraries(${PROJECT_NAME} PUBLIC glfw glm vulkan-1)