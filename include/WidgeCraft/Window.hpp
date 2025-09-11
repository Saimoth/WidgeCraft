#pragma once
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

    private:
        GLFWwindow* m_window;
    };

} // namespace WidgeCraft
