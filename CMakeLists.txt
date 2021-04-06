cmake_minimum_required(VERSION 3.16)
project(FLOW_MAP_VISUALIZATION)
set(CMAKE_CXX_STANDARD 11)

find_package(rkcommon REQUIRED)
find_package(ospray REQUIRED)
find_package(OpenGL REQUIRED)

## path to glfw3
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/glfw-3.3.3")
include_directories("${CMAKE_SOURCE_DIR}/externals/glfw-3.3.3/include")
## path to gl3w
include_directories("${CMAKE_SOURCE_DIR}/externals/gl3w/include")
add_library(gl3w ${CMAKE_SOURCE_DIR}/externals/gl3w/src/gl3w.c)
set_target_properties(gl3w PROPERTIES COMPILE_FLAGS "-w")
target_include_directories(gl3w PUBLIC "$<BUILD_INTERFACE:""externals/gl3w/include;"">")
# path to imgui
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/imgui")
include_directories("${CMAKE_SOURCE_DIR}/externals/imgui")
## Other useful libs
add_subdirectory("${CMAKE_SOURCE_DIR}/utils")
include_directories("${CMAKE_SOURCE_DIR}/utils")

add_executable(viewer
               main.cpp
)   

target_link_libraries(viewer 
                      glfw
                      gl3w
                      imgui
                      utils
                      rkcommon::rkcommon
                      ospray::ospray)   

