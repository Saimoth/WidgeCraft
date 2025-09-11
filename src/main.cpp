#include "WidgeCraft/Window.hpp"

// Order matters: glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

int main() {
    try {
        WidgeCraft::Window window(800, 600, "WidgeCraft Engine");

        while (!window.shouldClose()) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glfwSwapBuffers(window.getNativeHandle());
            window.pollEvents();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cin.get();
        return -1;
    }

    return 0;
}
