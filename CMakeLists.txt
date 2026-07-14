cmake_minimum_required(VERSION 3.25)
project(WidgeCraft VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WIDGECRAFT_BUILD_DEMO "Build the interactive WidgeCraft showcase" ON)
option(WIDGECRAFT_BUILD_SANDBOX "Build the interactive 3D raycast sandbox" ON)
option(WIDGECRAFT_BUILD_TESTS "Build WidgeCraft unit tests" ON)

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

# ---------------- Engine library ----------------
add_library(widgecraft_engine STATIC
    src/Window.cpp
    src/Input.cpp
    src/TextRenderer.cpp
    src/ShapeRenderer.cpp
    src/Frame.cpp
    src/Widget.cpp
    src/Raycast.cpp
    src/WidgeCraft.cpp
)

add_library(WidgeCraft::Engine ALIAS widgecraft_engine)

set_target_properties(widgecraft_engine PROPERTIES
    OUTPUT_NAME WidgeCraft
    FOLDER "WidgeCraft"
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

# ---------------- Interactive showcase ----------------
if(WIDGECRAFT_BUILD_DEMO)
    add_executable(widgecraft src/main.cpp)
    set_target_properties(widgecraft PROPERTIES FOLDER "Examples")
    target_link_libraries(widgecraft PRIVATE WidgeCraft::Engine)
    widgecraft_enable_warnings(widgecraft)

    add_custom_command(TARGET widgecraft POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:widgecraft>/assets
        COMMENT "Copying WidgeCraft assets"
    )
endif()

# ---------------- Interactive raycast sandbox test ----------------
if(WIDGECRAFT_BUILD_SANDBOX)
    add_executable(widgecraft_3d_sandbox tests/ThreeDSandbox.cpp)
    set_target_properties(widgecraft_3d_sandbox PROPERTIES FOLDER "Tests")
    target_link_libraries(widgecraft_3d_sandbox PRIVATE WidgeCraft::Engine)
    widgecraft_enable_warnings(widgecraft_3d_sandbox)

    # Make the generated Visual Studio solution launch the sandbox by default.
    set_property(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT widgecraft_3d_sandbox
    )

    add_custom_command(TARGET widgecraft_3d_sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:widgecraft_3d_sandbox>/assets
        COMMENT "Copying WidgeCraft assets for 3D sandbox"
    )
endif()

# ---------------- Tests ----------------
if(WIDGECRAFT_BUILD_TESTS)
    include(CTest)
    enable_testing()

    add_executable(widgecraft_raycast_tests
        tests/RaycastTests.cpp
        src/Raycast.cpp
    )
    set_target_properties(widgecraft_raycast_tests PROPERTIES FOLDER "Tests")
    target_include_directories(widgecraft_raycast_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    widgecraft_enable_warnings(widgecraft_raycast_tests)

    add_test(NAME WidgeCraft.Raycast COMMAND widgecraft_raycast_tests)
endif()
