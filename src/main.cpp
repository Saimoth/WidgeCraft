#include "WidgeCraft/WidgeCraft.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try {
        WidgeCraft::WidgeCraft wc("WidgeCraft Engine", 800, 600);

        const std::string rootFrameName = wc.getRootFrameName();
        const std::string headerFrameName = "Header";
        const std::string titleWidgetName = "TitleLabel";
        const std::string actionWidgetName = "ActionButton";

        wc.onStartup([&](WidgeCraft::WidgeCraft& app) {
            app.frameSetBorderVisible(rootFrameName, false);

            app.addFrame(headerFrameName);
            app.frameSetBackgroundVisible(headerFrameName, false);
            app.frameSetBorderVisible(headerFrameName, false);

            app.addLabel(headerFrameName, titleWidgetName, "WidgeCraft Engine");
            app.addButton(headerFrameName, actionWidgetName, "Start Crafting");

            app.widgetSetColor(headerFrameName, titleWidgetName, { 1.0f, 1.0f, 0.9f });
            app.widgetSetTextColor(headerFrameName, actionWidgetName, { 0.8f, 0.9f, 1.0f });
            app.widgetSetBackgroundVisible(headerFrameName, actionWidgetName, false);

            const float baseTextSize = app.getTextRenderer().getBasePixelHeight();
            app.widgetSetTextSize(headerFrameName, titleWidgetName, baseTextSize);
            app.widgetSetTextSize(headerFrameName, actionWidgetName, baseTextSize * 0.5f);
        });

        wc.onUpdate([&](WidgeCraft::WidgeCraft& app) {
            auto& window = app.getWindow();
            auto& textRenderer = app.getTextRenderer();

            const float margin = 40.0f;
            const float headerHeight = textRenderer.getLineHeight() * 3.0f;
            app.frameSetSize(headerFrameName, static_cast<float>(window.getWidth()) - margin * 2.0f, headerHeight);
            app.frameSetPosition(headerFrameName, margin, static_cast<float>(window.getHeight()) - headerHeight - margin);

            const float headerTopBaseline = app.frameGetSize(headerFrameName).y - textRenderer.getAscent() - 10.0f;
            app.widgetSetPosition(headerFrameName, titleWidgetName, { 0.0f, headerTopBaseline });
            app.widgetSetPosition(headerFrameName, actionWidgetName, { 0.0f, headerTopBaseline - textRenderer.getLineHeight() });
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

