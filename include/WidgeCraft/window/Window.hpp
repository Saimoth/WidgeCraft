#pragma once

#include "WidgeCraft/Frame.hpp"
#include "WidgeCraft/Input.hpp"

#include <string>

struct GLFWwindow;

namespace WidgeCraft {

    class Window {
    public:
        Window(int width, int height, const std::string& title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool shouldClose() const;
        void requestClose();
        void pollEvents();
        void swapBuffers() const;

        void setVSync(bool enabled);
        bool isVSyncEnabled() const { return m_vsyncEnabled; }
        void setTitle(const std::string& title);

        GLFWwindow* getNativeHandle() const { return m_window; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }
        float getAspectRatio() const;

        Input& getInput() { return m_input; }
        const Input& getInput() const { return m_input; }

        Frame& getRootFrame() { return m_rootFrame; }
        const Frame& getRootFrame() const { return m_rootFrame; }

    private:
        GLFWwindow* m_window = nullptr;
        int m_width = 0;
        int m_height = 0;
        bool m_vsyncEnabled = true;
        Input m_input;
        Frame m_rootFrame;

        void updateSize(int width, int height);

        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers);
        static void cursorPositionCallback(GLFWwindow* window, double x, double y);
        static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    };

} // namespace WidgeCraft
