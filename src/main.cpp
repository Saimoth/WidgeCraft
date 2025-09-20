#include "WidgeCraft/Window.hpp"
#include "WidgeCraft/TextRenderer.hpp"

// Order matters: glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

int main() {
    try {
        WidgeCraft::Window window(800, 600, "WidgeCraft Engine");
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const std::string fontPath = "assets/fonts/Roboto-Regular.ttf";
        WidgeCraft::TextRenderer textRenderer(window.getWidth(), window.getHeight(), fontPath, 64.0f);

        while (!window.shouldClose()) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            textRenderer.setScreenSize(window.getWidth(), window.getHeight());
            const float margin = 40.0f;
            const float baseline = static_cast<float>(window.getHeight()) - textRenderer.getAscent() - margin;
            textRenderer.renderText(
                "Signed Distance Field Text\nRendering from TTF fonts",
                margin,
                baseline,
                1.0f,
                { 1.0f, 1.0f, 1.0f });

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
