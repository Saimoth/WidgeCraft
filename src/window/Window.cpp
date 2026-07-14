#include "WidgeCraft/Window.hpp"

// GLAD must be included before GLFW.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace WidgeCraft {

    Window::Window(int width, int height, const std::string& title)
        : m_width(width)
        , m_height(height)
        , m_rootFrame("Window Frame") {

        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Window dimensions must be positive");
        }

        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, Window::framebufferSizeCallback);
        glfwSetKeyCallback(m_window, Window::keyCallback);
        glfwSetMouseButtonCallback(m_window, Window::mouseButtonCallback);
        glfwSetCursorPosCallback(m_window, Window::cursorPositionCallback);
        glfwSetScrollCallback(m_window, Window::scrollCallback);

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }

        int framebufferWidth = width;
        int framebufferHeight = height;
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
        updateSize(framebufferWidth, framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        setVSync(true);

        m_rootFrame.setDeletable(false);
        m_rootFrame.setAnchor(Anchor::BottomLeft);
        m_rootFrame.setBackgroundVisible(false);
        m_rootFrame.setBorderVisible(false);
        m_rootFrame.setPosition(0.0f, 0.0f);
        m_rootFrame.setSize(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
    }

    Window::~Window() {
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }

    bool Window::shouldClose() const {
        return m_window == nullptr || glfwWindowShouldClose(m_window) != 0;
    }

    void Window::requestClose() {
        if (m_window) {
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
    }

    void Window::pollEvents() {
        m_input.beginFrame();
        glfwPollEvents();
    }

    void Window::swapBuffers() const {
        if (m_window) {
            glfwSwapBuffers(m_window);
        }
    }

    void Window::setVSync(bool enabled) {
        m_vsyncEnabled = enabled;
        glfwSwapInterval(enabled ? 1 : 0);
    }

    void Window::setTitle(const std::string& title) {
        if (m_window) {
            glfwSetWindowTitle(m_window, title.c_str());
        }
    }

    float Window::getAspectRatio() const {
        return m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
    }

    void Window::updateSize(int width, int height) {
        m_width = width;
        m_height = height;
        m_rootFrame.setSize(static_cast<float>(width), static_cast<float>(height));
    }

    void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
            self->updateSize(width, height);
            glViewport(0, 0, width, height);
        }
    }

    void Window::keyCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers) {
        (void)scanCode;
        (void)modifiers;
        if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                self->m_input.setKey(key, true);
            } else if (action == GLFW_RELEASE) {
                self->m_input.setKey(key, false);
            }
        }
    }

    void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers) {
        (void)modifiers;
        if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
            if (action == GLFW_PRESS) {
                self->m_input.setMouseButton(button, true);
            } else if (action == GLFW_RELEASE) {
                self->m_input.setMouseButton(button, false);
            }
        }
    }

    void Window::cursorPositionCallback(GLFWwindow* window, double x, double y) {
        if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
            int windowWidth = 1;
            int windowHeight = 1;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);

            const float scaleX = windowWidth > 0 ? static_cast<float>(self->m_width) / static_cast<float>(windowWidth) : 1.0f;
            const float scaleY = windowHeight > 0 ? static_cast<float>(self->m_height) / static_cast<float>(windowHeight) : 1.0f;
            self->m_input.setMousePosition(
                static_cast<float>(x) * scaleX,
                static_cast<float>(self->m_height) - static_cast<float>(y) * scaleY);
        }
    }

    void Window::scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
        if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
            self->m_input.addScroll(static_cast<float>(xOffset), static_cast<float>(yOffset));
        }
    }

} // namespace WidgeCraft
