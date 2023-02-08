cmake_minimum_required(VERSION 3.20)

include(tools/CMakeLists.txt)

project(RDK)
# targets
if(DEBUG)
    add_definitions(-DDEBUG=1)
endif(DEBUG)

if(IMGUI)
    add_definitions(-DIMGUI=1)
endif(IMGUI)
# sources
file(GLOB_RECURSE PROJECT_SRC cpp/*.cpp include/*.h vendor/stb/*.h
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/backends/imgui_impl_glfw.cpp
        vendor/imgui/backends/imgui_impl_vulkan.cpp
        vendor/imgui/misc/cpp/imgui_stdlib.cpp
)
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
# STB Image loading
include_directories(vendor/stb)
# UI for runtime testing
include_directories(vendor/imgui)
# Original shaders
configure_file(shaders/shader.vert shaders/shader.vert COPYONLY)
configure_file(shaders/shader.frag shaders/shader.frag COPYONLY)
# Textures
configure_file(textures/statue.jpg textures/statue.jpg COPYONLY)
# Assets
replace_dirs(assets assets)
# links
target_link_libraries(${PROJECT_NAME} PUBLIC glfw glm vulkan-1)
dynamic_link(${PROJECT_NAME} shaderc_shared)
dynamic_link(${PROJECT_NAME} spirv-cross-c-shared)
dynamic_link(${PROJECT_NAME} SPIRV-Tools-shared)