#include "WidgeCraft/WidgeCraft.hpp"
#include "WidgeCraft/Widget.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try {
        WidgeCraft::WidgeCraft wc("WidgeCraft Engine", 800, 600);

        WidgeCraft::Frame& rootFrame = wc.getRootFrame();
        rootFrame.setBorderVisible(false);

        WidgeCraft::Frame& headerFrame = rootFrame.createChildFrame("Header");
        headerFrame.setBackgroundVisible(false);
        headerFrame.setBorderVisible(false);

        auto& titleLabel = headerFrame.addWidget<WidgeCraft::Label>("TitleLabel", "WidgeCraft Engine");
        auto& actionButton = headerFrame.addWidget<WidgeCraft::Button>("ActionButton", "Start Crafting");

        titleLabel.setColor({ 1.0f, 1.0f, 0.9f });
        actionButton.setTextColor({ 0.8f, 0.9f, 1.0f });
        actionButton.setBackgroundVisible(false);

        const float baseTextSize = wc.getTextRenderer().getBasePixelHeight();
        titleLabel.setTextSize(baseTextSize);
        actionButton.setTextSize(baseTextSize * 0.5f);

        wc.setUpdateCallback([&](WidgeCraft::WidgeCraft& app) {
            auto& window = app.getWindow();
            auto& textRenderer = app.getTextRenderer();

            const float margin = 40.0f;
            const float headerHeight = textRenderer.getLineHeight() * 3.0f;
            headerFrame.setSize(static_cast<float>(window.getWidth()) - margin * 2.0f, headerHeight);
            headerFrame.setPosition(margin, static_cast<float>(window.getHeight()) - headerHeight - margin);

            const float headerTopBaseline = headerFrame.getSize().y - textRenderer.getAscent() - 10.0f;
            titleLabel.setPosition(0.0f, headerTopBaseline);
            actionButton.setPosition(0.0f, headerTopBaseline - textRenderer.getLineHeight());
        });

        wc.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cin.get();
        return -1;
    }

    return 0;
}

