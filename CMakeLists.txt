cmake_minimum_required(VERSION 3.25)
project(WidgeCraft VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(WIDGECRAFT_BUILD_SANDBOX "Build the WidgeCraft UI sandbox" ON)

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
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# ---------------- GLAD ----------------
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glad)

function(widgecraft_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

set(WIDGECRAFT_ENGINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/WidgeCraft.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/input/Input.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/window/Window.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/ui/Frame.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/ui/Widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/scene/Scene.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/scene/SceneViewport2D.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/scene/Raycast.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/render/ShaderProgram.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/primitives/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/primitives/Shapes2D.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/primitives/Shapes3D.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WidgeCraft/primitives/TextRenderer.hpp
)

set(WIDGECRAFT_ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/WidgeCraft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/Input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/Raycast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/ShaderProgram.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/primitives/Shapes2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/primitives/Shapes3D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/primitives/TextRenderer.cpp
)

# ---------------- Engine library ----------------
add_library(widgecraft_engine STATIC
    ${WIDGECRAFT_ENGINE_HEADERS}
    ${WIDGECRAFT_ENGINE_SOURCES}
)
add_library(WidgeCraft::Engine ALIAS widgecraft_engine)

set_target_properties(widgecraft_engine PROPERTIES
    OUTPUT_NAME WidgeCraft
    FOLDER "Engine"
)

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PREFIX "Header Files"
    FILES ${WIDGECRAFT_ENGINE_HEADERS}
)
source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Source Files"
    FILES ${WIDGECRAFT_ENGINE_SOURCES}
)

target_include_directories(widgecraft_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_definitions(widgecraft_engine
    PUBLIC
        WIDGECRAFT_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_link_libraries(widgecraft_engine PUBLIC glfw glad)
widgecraft_enable_warnings(widgecraft_engine)

# ---------------- Launchable sandboxes ----------------
if(WIDGECRAFT_BUILD_SANDBOX)
    add_executable(widgecraft_ui_sandbox
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/UiDemo.cpp
    )
    set_target_properties(widgecraft_ui_sandbox PROPERTIES
        FOLDER "Sandbox"
    )
    target_link_libraries(widgecraft_ui_sandbox PRIVATE WidgeCraft::Engine)
    widgecraft_enable_warnings(widgecraft_ui_sandbox)

    set_property(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT widgecraft_ui_sandbox
    )

    add_custom_command(TARGET widgecraft_ui_sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:widgecraft_ui_sandbox>/assets
        COMMENT "Copying WidgeCraft assets for UI sandbox"
    )
endif()
