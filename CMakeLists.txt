cmake_minimum_required(VERSION 3.20)
project(Rect)
# targets
if(DEBUG)
    add_definitions(-DDEBUG=1)
endif(DEBUG)
# sources
file(GLOB_RECURSE PROJECT_SRC cpp/*.cpp cpp/*.c)
add_executable(${PROJECT_NAME} ${PROJECT_SRC})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
# includes
include_directories(include)
# GLFW
add_subdirectory(vendor/glfw)
include_directories(vendor/glfw/include)
include_directories(vendor/glfw/src)
# GLM
add_subdirectory(vendor/glm)
include_directories(vendor/glm/glm)
# Vulkan
include_directories(vendor/vulkan/Include)
configure_file(vulkan-1.lib vulkan-1.lib COPYONLY)
# Original shaders
configure_file(shaders/shader.vert shaders/shader.vert COPYONLY)
configure_file(shaders/shader.frag shaders/shader.frag COPYONLY)
# SPIR-V
configure_file(spirv/shader_vert.spv spirv/shader_vert.spv COPYONLY)
configure_file(spirv/shader_frag.spv spirv/shader_frag.spv COPYONLY)
# links
target_link_libraries(${PROJECT_NAME} PUBLIC glfw glm vulkan-1)