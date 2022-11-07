# CMake version and project's name
cmake_minimum_required(VERSION 3.16)
project(GLF3D)

# Headers and source files
include_directories(GLOB HEADER_FILES "${PROJECT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/*.cpp")

# Generates executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Specify CPP version
set(CMAKE_CXX_STANDARD 14)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# Pre compiled headers
target_precompile_headers(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/PCH.h")

# Windows ONLY
if (WIN32)
	target_include_directories(${PROJECT_NAME} PRIVATE "Dependencies/glm"
                                                       #Dependencies/stb
                                                       "Dependencies/glfw/include",
                                                       "Dependencies/glew/include")

    target_link_directories(${PROJECT_NAME} PRIVATE "Dependencies/glfw/lib-vc2022"
                                                    "Dependencies/glew/lib/Release/x64")

    target_link_libraries(${PROJECT_NAME} PRIVATE glfw3.lib
                                                  glew32.lib
                                                  opengl32.lib)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/Dependencies/glew/bin/Release/x64/glew32.dll" "${CMAKE_BINARY_DIR}/glew32.dll"
                    COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/Shaders" "${CMAKE_BINARY_DIR}/Shaders"
                    COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/Textures" "${CMAKE_BINARY_DIR}/Textures")

# Linux ONLY
else()
    # Find packages
	find_package(glfw3  REQUIRED)
	find_package(GLEW   REQUIRED)
	find_package(OpenGL REQUIRED)

	# Static libraries
	target_link_libraries(${PROJECT_NAME} GLEW::glew glfw OpenGL::OpenGL)

endif()