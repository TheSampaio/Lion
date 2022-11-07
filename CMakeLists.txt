# CMake version and project's name
cmake_minimum_required(VERSION 3.16)
project(GLF3D)

# Headers and source files
include_directories(GLOB HEADER_FILES "${PROJECT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/*.cpp")

# Generates executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Pre compiled headers
target_precompile_headers(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/PCH.h")

# Find packages (Linux ONLY)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

# Static libraries
target_link_libraries(${PROJECT_NAME} GLEW::glew glfw OpenGL::OpenGL)
