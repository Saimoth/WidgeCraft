#include "WidgeCraft/Window.hpp"
#include "WidgeCraft/Frame.hpp"
#include "WidgeCraft/Widget.hpp"
#include "WidgeCraft/TextRenderer.hpp"

// Order matters: glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    try {
        WidgeCraft::Window window(800, 600, "WidgeCraft Engine");
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const std::filesystem::path fontPath = std::filesystem::path(WIDGECRAFT_ASSET_DIR) / "fonts/Roboto-Regular.ttf";
        WidgeCraft::TextRenderer textRenderer(window.getWidth(), window.getHeight(), fontPath.string(), 64.0f);

        WidgeCraft::Frame& rootFrame = window.getRootFrame();
        rootFrame.setBackgroundVisible(false);
        rootFrame.setBorderVisible(false);

        WidgeCraft::Frame& headerFrame = rootFrame.createChildFrame("Header");
        headerFrame.setBackgroundVisible(false);
        headerFrame.setBorderVisible(false);

        auto& titleLabel = headerFrame.addWidget<WidgeCraft::Label>("TitleLabel", "WidgeCraft Engine");
        auto& actionButton = headerFrame.addWidget<WidgeCraft::Button>("ActionButton", "Start Crafting");

        titleLabel.setColor({ 1.0f, 1.0f, 0.9f });
        actionButton.setTextColor({ 0.8f, 0.9f, 1.0f });
        actionButton.setBackgroundVisible(false);

        const float baseTextSize = textRenderer.getBasePixelHeight();
        titleLabel.setTextSize(baseTextSize);
        actionButton.setTextSize(baseTextSize * 0.5f);

        while (!window.shouldClose()) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            textRenderer.setScreenSize(window.getWidth(), window.getHeight());
            const float margin = 40.0f;
            const float headerHeight = textRenderer.getLineHeight() * 3.0f;
            headerFrame.setSize(static_cast<float>(window.getWidth()) - margin * 2.0f, headerHeight);
            headerFrame.setPosition(margin, static_cast<float>(window.getHeight()) - headerHeight - margin);

            const float headerTopBaseline = headerFrame.getSize().y - textRenderer.getAscent() - 10.0f;
            titleLabel.setPosition(0.0f, headerTopBaseline);
            actionButton.setPosition(0.0f, headerTopBaseline - textRenderer.getLineHeight());

            rootFrame.render(textRenderer);

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
