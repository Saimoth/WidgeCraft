#include "WidgeCraft/WidgeCraft.hpp"

#include <exception>
#include <iostream>
#include <string>

int main() {
    try {
        WidgeCraft::WidgeCraft app(
            "WidgeCraft UI Sandbox",
            1100,
            720);
        app.setClearColor({ 0.030f, 0.040f, 0.060f, 1.0f });

        auto& root = app.getRootFrame();
        root.setBackgroundVisible(false);

        auto& panel = root.createChildFrame("Main Panel");
        panel.setAnchor(WidgeCraft::Anchor::Center);
        panel.setPosition(0.0f, 0.0f);
        panel.setSize(520.0f, 370.0f);
        panel.setBackgroundColor({ 0.070f, 0.090f, 0.125f, 0.97f });
        panel.setBorderVisible(true);
        panel.setBorderColor({ 0.28f, 0.48f, 0.72f, 1.0f });
        panel.setBorderThickness(2.0f);

        auto& title = panel.addWidget<WidgeCraft::Label>(
            "Title",
            "WidgeCraft UI Sandbox");
        title.setAnchor(WidgeCraft::Anchor::TopLeft);
        title.setPosition(24.0f, 20.0f);
        title.setTextSize(30.0f);
        title.setColor({ 0.74f, 0.90f, 1.0f, 1.0f });

        auto& subtitle = panel.addWidget<WidgeCraft::Label>(
            "Subtitle",
            "Frames, widgets, Shapes2D and SDF text");
        subtitle.setAnchor(WidgeCraft::Anchor::TopLeft);
        subtitle.setPosition(24.0f, 62.0f);
        subtitle.setTextSize(16.0f);
        subtitle.setColor({ 0.65f, 0.71f, 0.82f, 1.0f });

        auto& statusCard = panel.createChildFrame("Status Card");
        statusCard.setAnchor(WidgeCraft::Anchor::TopLeft);
        statusCard.setPosition(24.0f, 108.0f);
        statusCard.setSize(472.0f, 92.0f);
        statusCard.setBackgroundColor({ 0.045f, 0.060f, 0.085f, 1.0f });
        statusCard.setBorderVisible(true);
        statusCard.setBorderColor({ 0.16f, 0.24f, 0.34f, 1.0f });
        statusCard.setBorderThickness(1.0f);

        auto& status = statusCard.addWidget<WidgeCraft::Label>(
            "Status",
            "The UI is ready.");
        status.setAnchor(WidgeCraft::Anchor::CenterLeft);
        status.setPosition(18.0f, 0.0f);
        status.setSize(430.0f, 34.0f);
        status.setTextSize(18.0f);
        status.setColor({ 0.66f, 0.88f, 0.72f, 1.0f });

        bool showDecoration = true;
        int clickCount = 0;

        auto& action = panel.addWidget<WidgeCraft::Button>(
            "Action",
            "Press me");
        action.setAnchor(WidgeCraft::Anchor::BottomLeft);
        action.setPosition(24.0f, 30.0f);
        action.setSize(138.0f, 46.0f);
        action.setTextSize(17.0f);
        action.setOnClick([&]() {
            ++clickCount;
            status.setText(
                "Button pressed "
                + std::to_string(clickCount)
                + (clickCount == 1 ? " time." : " times."));
        });

        auto& reset = panel.addWidget<WidgeCraft::Button>(
            "Reset",
            "Reset");
        reset.setAnchor(WidgeCraft::Anchor::BottomLeft);
        reset.setPosition(176.0f, 30.0f);
        reset.setSize(112.0f, 46.0f);
        reset.setTextSize(17.0f);
        reset.setOnClick([&]() {
            clickCount = 0;
            status.setText("Counter reset.");
        });

        auto& decoration = panel.addWidget<WidgeCraft::Checkbox>(
            "Decoration",
            "Show scene decoration",
            showDecoration);
        decoration.setAnchor(WidgeCraft::Anchor::BottomRight);
        decoration.setPosition(24.0f, 40.0f);
        decoration.setTextSize(16.0f);
        decoration.setOnChanged([&](bool checked) {
            showDecoration = checked;
            status.setText(
                checked
                ? "Scene decoration enabled."
                : "Scene decoration hidden.");
        });

        app.setRenderCallback([&](WidgeCraft::WidgeCraft& engine) {
            if (!showDecoration) {
                return;
            }

            auto& shapes = engine.getShapes2D();
            const float width = static_cast<float>(
                engine.getWindow().getWidth());
            const float height = static_cast<float>(
                engine.getWindow().getHeight());

            WidgeCraft::ShapeStyle2D circleStyle;
            circleStyle.fillColor = { 0.06f, 0.20f, 0.30f, 0.75f };
            circleStyle.edgeColor = { 0.25f, 0.68f, 0.92f, 0.9f };
            circleStyle.edgeThickness = 3.0f;
            circleStyle.edgeVisible = true;
            shapes.drawCircle(
                { width * 0.18f, height * 0.74f },
                92.0f,
                circleStyle);

            WidgeCraft::ShapeStyle2D rectangleStyle;
            rectangleStyle.fillColor = { 0.16f, 0.08f, 0.24f, 0.70f };
            rectangleStyle.edgeColor = { 0.66f, 0.38f, 0.90f, 0.95f };
            rectangleStyle.edgeThickness = 4.0f;
            rectangleStyle.edgeVisible = true;
            shapes.drawRect(
                { width * 0.74f, height * 0.18f, 170.0f, 116.0f },
                rectangleStyle);

            WidgeCraft::ShapeStyle2D triangleStyle;
            triangleStyle.fillColor = { 0.10f, 0.28f, 0.18f, 0.72f };
            triangleStyle.edgeColor = { 0.35f, 0.84f, 0.56f, 0.95f };
            triangleStyle.edgeThickness = 4.0f;
            triangleStyle.edgeVisible = true;
            shapes.drawTriangle(
                { width * 0.79f, height * 0.78f },
                { width * 0.90f, height * 0.58f },
                { width * 0.68f, height * 0.58f },
                triangleStyle);
        });

        app.Run(60);
    } catch (const std::exception& exception) {
        std::cerr << "WidgeCraft UI sandbox failed: "
            << exception.what() << '\n';
        return 1;
    }

    return 0;
}
