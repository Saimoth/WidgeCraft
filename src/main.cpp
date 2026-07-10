#include "WidgeCraft/WidgeCraft.hpp"
#include "WidgeCraft/Widget.hpp"

#include <cmath>
#include <iostream>
#include <string>

int main() {
    try {
        WidgeCraft::WidgeCraft app("WidgeCraft Engine", 1100, 720);
        app.setClearColor({ 0.035f, 0.045f, 0.065f, 1.0f });

        auto& root = app.getRootFrame();
        root.setBackgroundVisible(false);

        auto& panel = root.createChildFrame("Demo Panel");
        panel.setAnchor(WidgeCraft::Anchor::TopLeft);
        panel.setPosition(24.0f, 24.0f);
        panel.setSize(360.0f, 236.0f);
        panel.setBackgroundColor({ 0.075f, 0.095f, 0.13f, 0.96f });
        panel.setBorderVisible(true);
        panel.setBorderColor({ 0.26f, 0.36f, 0.50f, 1.0f });
        panel.setBorderThickness(1.0f);

        auto& title = panel.addWidget<WidgeCraft::Label>("Title", "WidgeCraft");
        title.setAnchor(WidgeCraft::Anchor::TopLeft);
        title.setPosition(18.0f, 16.0f);
        title.setTextSize(34.0f);
        title.setColor({ 0.72f, 0.90f, 1.0f, 1.0f });

        auto& subtitle = panel.addWidget<WidgeCraft::Label>(
            "Subtitle",
            "Retained UI • batched primitives • SDF text");
        subtitle.setAnchor(WidgeCraft::Anchor::TopLeft);
        subtitle.setPosition(18.0f, 62.0f);
        subtitle.setTextSize(16.0f);
        subtitle.setColor({ 0.68f, 0.73f, 0.82f, 1.0f });

        bool showGrid = true;
        auto& gridCheckbox = panel.addWidget<WidgeCraft::Checkbox>("Grid Toggle", "Show primitive grid", showGrid);
        gridCheckbox.setAnchor(WidgeCraft::Anchor::TopLeft);
        gridCheckbox.setPosition(18.0f, 102.0f);
        gridCheckbox.setTextSize(17.0f);
        gridCheckbox.setOnChanged([&](bool checked) {
            showGrid = checked;
        });

        auto& status = panel.addWidget<WidgeCraft::Label>("Status", "Button ready");
        status.setAnchor(WidgeCraft::Anchor::BottomLeft);
        status.setPosition(176.0f, 28.0f);
        status.setTextSize(16.0f);
        status.setColor({ 0.62f, 0.78f, 0.67f, 1.0f });

        int clickCount = 0;
        auto& action = panel.addWidget<WidgeCraft::Button>("Action", "Craft something");
        action.setAnchor(WidgeCraft::Anchor::BottomLeft);
        action.setPosition(18.0f, 18.0f);
        action.setSize(142.0f, 44.0f);
        action.setTextSize(17.0f);
        action.setOnClick([&]() {
            ++clickCount;
            status.setText("Clicked " + std::to_string(clickCount) + (clickCount == 1 ? " time" : " times"));
        });

        app.setRenderCallback([&](WidgeCraft::WidgeCraft& engine) {
            auto& shapes = engine.getShapeRenderer();
            auto& text = engine.getTextRenderer();
            const float width = static_cast<float>(engine.getWindow().getWidth());
            const float height = static_cast<float>(engine.getWindow().getHeight());

            if (showGrid) {
                for (float x = 0.0f; x <= width; x += 40.0f) {
                    shapes.drawLine(x, 0.0f, x, height, 1.0f, { 0.10f, 0.13f, 0.18f, 0.75f });
                }
                for (float y = 0.0f; y <= height; y += 40.0f) {
                    shapes.drawLine(0.0f, y, width, y, 1.0f, { 0.10f, 0.13f, 0.18f, 0.75f });
                }
            }

            const float centerX = width * 0.67f;
            const float centerY = height * 0.48f;
            const float angle = static_cast<float>(engine.getElapsedTime()) * 0.8f;

            shapes.drawFilledCircle(centerX, centerY, 86.0f, { 0.08f, 0.34f, 0.46f, 0.65f });
            shapes.drawCircleOutline(centerX, centerY, 86.0f, 4.0f, { 0.38f, 0.83f, 1.0f, 1.0f });
            shapes.drawLine(
                centerX,
                centerY,
                centerX + std::cos(angle) * 72.0f,
                centerY + std::sin(angle) * 72.0f,
                7.0f,
                { 1.0f, 0.72f, 0.28f, 1.0f });
            shapes.drawPoint(centerX, centerY, 16.0f, { 1.0f, 0.95f, 0.78f, 1.0f });

            shapes.drawTriangle(
                { centerX - 145.0f, centerY - 150.0f },
                { centerX - 35.0f, centerY - 150.0f },
                { centerX - 90.0f, centerY - 55.0f },
                { 0.62f, 0.28f, 0.82f, 0.85f });
            shapes.drawRectOutline(
                centerX + 25.0f,
                centerY - 150.0f,
                150.0f,
                94.0f,
                5.0f,
                { 0.32f, 0.80f, 0.55f, 1.0f });

            text.renderText("SDF at 12 px", centerX - 175.0f, height - 70.0f, 12.0f, { 0.78f, 0.83f, 0.92f, 1.0f });
            text.renderText("SDF at 30 px", centerX - 175.0f, height - 112.0f, 30.0f, { 0.80f, 0.92f, 1.0f, 1.0f });
            text.renderText("SDF", centerX - 175.0f, 72.0f, 92.0f, { 0.20f, 0.62f, 0.84f, 0.34f });
        });

        app.Run(60);
    } catch (const std::exception& exception) {
        std::cerr << "WidgeCraft failed: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
