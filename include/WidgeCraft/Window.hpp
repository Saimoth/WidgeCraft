#pragma once
#include "WidgeCraft/Frame.hpp"

#include <string>

// Forward declaration of GLFW window type
struct GLFWwindow;

namespace WidgeCraft {

    class Window {
    public:
        Window(int width, int height, const std::string& title);
        ~Window();

        bool shouldClose() const;
        void pollEvents() const;

        // Needed so we can swap buffers in main
        GLFWwindow* getNativeHandle() const { return m_window; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

        Frame& getRootFrame() { return m_rootFrame; }
        const Frame& getRootFrame() const { return m_rootFrame; }

    private:
        GLFWwindow* m_window;
        int m_width;
        int m_height;
        Frame m_rootFrame;

        void updateSize(int width, int height);
        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    };

} // namespace WidgeCraft
