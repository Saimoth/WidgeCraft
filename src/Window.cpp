#include "WidgeCraft/Window.hpp"

// Order matters: glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>

namespace WidgeCraft {

    Window::Window(int width, int height, const std::string& title)
        : m_window(nullptr), m_width(width), m_height(height) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Ask for OpenGL 3.3 core
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, Window::framebufferSizeCallback);

        // Load OpenGL function pointers via GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        glViewport(0, 0, width, height);

        std::cout << "Window created: " << title << " (" << width << "x" << height << ")\n";
    }

    Window::~Window() {
        if (m_window) {
            glfwDestroyWindow(m_window);
            glfwTerminate();
        }
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::pollEvents() const {
        glfwPollEvents();
    }

    void Window::updateSize(int width, int height) {
        m_width = width;
        m_height = height;
    }

    void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
            self->updateSize(width, height);
            glViewport(0, 0, width, height);
        }
    }

} // namespace WidgeCraft
