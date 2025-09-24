cmake_minimum_required(VERSION 3.25)
project(WidgeCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---------------- GLFW ----------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# ---------------- GLAD ----------------
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# ---------------- Executable ----------------
add_executable(widgecraft
    src/main.cpp
    src/Window.cpp
    src/TextRenderer.cpp
    src/Frame.cpp
    src/Widget.cpp
)

target_include_directories(widgecraft
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_definitions(widgecraft
    PRIVATE
        WIDGECRAFT_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_link_libraries(widgecraft PRIVATE glfw glad)

# ---------------- Warnings ----------------
if(MSVC)
    target_compile_options(widgecraft PRIVATE /W4 /permissive-)
else()
    target_compile_options(widgecraft PRIVATE -Wall -Wextra -Wpedantic)
endif()
