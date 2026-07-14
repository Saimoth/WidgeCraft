#include "WidgeCraft/WidgeCraft.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

int main() {
    try {
        WidgeCraft::WidgeCraft app(
            "WidgeCraft Model Viewport Sandbox",
            1100,
            720);
        app.setClearColor({ 0.020f, 0.028f, 0.044f, 1.0f });

        WidgeCraft::ModelViewport modelViewport;
        modelViewport.getCamera2D().setPosition({ 0.0f, 0.0f });
        modelViewport.getCamera2D().setZoom(1.0f);
        modelViewport.getCamera3D().setPosition({ 6.0f, 4.5f, 9.0f });
        modelViewport.getCamera3D().setTarget({ 0.0f, 0.0f, 0.0f });
        modelViewport.getCamera3D().setPerspective(52.0f, 0.1f, 500.0f);

        auto& root = app.getRootFrame();
        root.setBackgroundVisible(false);

        auto& modelFrame = root.createChildFrame("Model Area");
        modelFrame.setAnchor(WidgeCraft::Anchor::BottomLeft);
        modelFrame.setPosition(28.0f, 28.0f);
        modelFrame.setSize(1044.0f, 664.0f);
        modelFrame.setBackgroundVisible(false);
        modelFrame.setBorderVisible(true);
        modelFrame.setBorderColor({ 0.22f, 0.43f, 0.68f, 1.0f });
        modelFrame.setBorderThickness(3.0f);

        auto& panel = root.createChildFrame("Controls");
        panel.setAnchor(WidgeCraft::Anchor::TopLeft);
        panel.setPosition(48.0f, 48.0f);
        panel.setSize(360.0f, 214.0f);
        panel.setBackgroundColor({ 0.055f, 0.072f, 0.105f, 0.96f });
        panel.setBorderVisible(true);
        panel.setBorderColor({ 0.25f, 0.46f, 0.70f, 1.0f });
        panel.setBorderThickness(2.0f);

        auto& title = panel.addWidget<WidgeCraft::Label>(
            "Title",
            "Model Viewport");
        title.setAnchor(WidgeCraft::Anchor::TopLeft);
        title.setPosition(20.0f, 16.0f);
        title.setTextSize(28.0f);
        title.setColor({ 0.76f, 0.91f, 1.0f, 1.0f });

        auto& subtitle = panel.addWidget<WidgeCraft::Label>(
            "Subtitle",
            "Clipped 2D, 3D and scene text");
        subtitle.setAnchor(WidgeCraft::Anchor::TopLeft);
        subtitle.setPosition(20.0f, 54.0f);
        subtitle.setTextSize(15.0f);
        subtitle.setColor({ 0.63f, 0.72f, 0.84f, 1.0f });

        auto& status = panel.addWidget<WidgeCraft::Label>(
            "Status",
            "Resize the window or change 2D zoom.");
        status.setAnchor(WidgeCraft::Anchor::TopLeft);
        status.setPosition(20.0f, 86.0f);
        status.setSize(320.0f, 28.0f);
        status.setTextSize(15.0f);
        status.setColor({ 0.67f, 0.88f, 0.72f, 1.0f });

        float cameraZoom = 1.0f;
        bool show3D = true;

        auto& zoomIn = panel.addWidget<WidgeCraft::Button>(
            "Zoom In",
            "Zoom +");
        zoomIn.setAnchor(WidgeCraft::Anchor::BottomLeft);
        zoomIn.setPosition(20.0f, 22.0f);
        zoomIn.setSize(92.0f, 42.0f);
        zoomIn.setTextSize(16.0f);
        zoomIn.setOnClick([&]() {
            cameraZoom = std::min(cameraZoom * 1.25f, 4.0f);
            status.setText("2D zoom: " + std::to_string(cameraZoom));
        });

        auto& zoomOut = panel.addWidget<WidgeCraft::Button>(
            "Zoom Out",
            "Zoom -");
        zoomOut.setAnchor(WidgeCraft::Anchor::BottomLeft);
        zoomOut.setPosition(124.0f, 22.0f);
        zoomOut.setSize(92.0f, 42.0f);
        zoomOut.setTextSize(16.0f);
        zoomOut.setOnClick([&]() {
            cameraZoom = std::max(cameraZoom / 1.25f, 0.25f);
            status.setText("2D zoom: " + std::to_string(cameraZoom));
        });

        auto& threeDToggle = panel.addWidget<WidgeCraft::Checkbox>(
            "3D Toggle",
            "Show 3D cubes",
            show3D);
        threeDToggle.setAnchor(WidgeCraft::Anchor::BottomRight);
        threeDToggle.setPosition(20.0f, 31.0f);
        threeDToggle.setTextSize(15.0f);
        threeDToggle.setOnChanged([&](bool checked) {
            show3D = checked;
            status.setText(
                checked ? "3D scene enabled." : "3D scene hidden.");
        });

        app.setRenderCallback([&](WidgeCraft::WidgeCraft& engine) {
            const float windowWidth =
                static_cast<float>(engine.getWindow().getWidth());
            const float windowHeight =
                static_cast<float>(engine.getWindow().getHeight());

            modelFrame.setSize(
                std::max(windowWidth - 56.0f, 40.0f),
                std::max(windowHeight - 56.0f, 40.0f));

            const WidgeCraft::Rect frameRect = modelFrame.getAbsoluteRect();
            modelViewport.setScreenRect({
                frameRect.x + 4.0f,
                frameRect.y + 4.0f,
                std::max(frameRect.width - 8.0f, 1.0f),
                std::max(frameRect.height - 8.0f, 1.0f)
            });
            modelViewport.getCamera2D().setZoom(cameraZoom);
            engine.useModelViewport(modelViewport);

            auto& shapes2D = engine.getShapes2D();
            auto& shapes3D = engine.getShapes3D();
            auto& text = engine.getTextRenderer();

            for (int x = -1200; x <= 1200; x += 80) {
                const bool major = (x % 400) == 0;
                shapes2D.drawLine(
                    static_cast<float>(x), -900.0f,
                    static_cast<float>(x), 900.0f,
                    major ? 2.0f : 1.0f,
                    major
                        ? WidgeCraft::Color{ 0.14f, 0.26f, 0.38f, 0.72f }
                        : WidgeCraft::Color{ 0.08f, 0.13f, 0.20f, 0.60f });
            }
            for (int y = -900; y <= 900; y += 80) {
                const bool major = (y % 400) == 0;
                shapes2D.drawLine(
                    -1200.0f, static_cast<float>(y),
                    1200.0f, static_cast<float>(y),
                    major ? 2.0f : 1.0f,
                    major
                        ? WidgeCraft::Color{ 0.14f, 0.26f, 0.38f, 0.72f }
                        : WidgeCraft::Color{ 0.08f, 0.13f, 0.20f, 0.60f });
            }

            WidgeCraft::ShapeStyle2D circleStyle;
            circleStyle.fillColor = { 0.06f, 0.30f, 0.42f, 0.72f };
            circleStyle.edgeColor = { 0.32f, 0.82f, 1.0f, 1.0f };
            circleStyle.edgeThickness = 4.0f;
            circleStyle.edgeVisible = true;
            shapes2D.drawCircle({ -330.0f, -180.0f }, 92.0f, circleStyle);

            WidgeCraft::ShapeStyle2D rectangleStyle;
            rectangleStyle.fillColor = { 0.24f, 0.09f, 0.34f, 0.72f };
            rectangleStyle.edgeColor = { 0.76f, 0.42f, 0.94f, 1.0f };
            rectangleStyle.edgeThickness = 4.0f;
            rectangleStyle.edgeVisible = true;
            shapes2D.drawRect(
                { 230.0f, -245.0f, 190.0f, 130.0f },
                rectangleStyle);

            WidgeCraft::ShapeStyle2D triangleStyle;
            triangleStyle.fillColor = { 0.08f, 0.34f, 0.20f, 0.74f };
            triangleStyle.edgeColor = { 0.38f, 0.92f, 0.58f, 1.0f };
            triangleStyle.edgeThickness = 4.0f;
            triangleStyle.edgeVisible = true;
            shapes2D.drawTriangle(
                { 315.0f, 260.0f },
                { 425.0f, 70.0f },
                { 205.0f, 70.0f },
                triangleStyle);

            modelViewport.drawWorldText2D(
                text,
                "2D world text",
                { -430.0f, 275.0f },
                28.0f,
                { 0.76f, 0.90f, 1.0f, 1.0f });

            if (show3D) {
                WidgeCraft::ShapeStyle3D cubeStyle;
                cubeStyle.fillColor = { 0.14f, 0.42f, 0.72f, 0.78f };
                cubeStyle.edgeColor = { 0.72f, 0.90f, 1.0f, 1.0f };
                cubeStyle.edgeThickness = 2.0f;
                cubeStyle.edgeVisible = true;

                shapes3D.drawCube({ -1.7f, 0.0f, 0.0f }, 1.8f, cubeStyle);
                cubeStyle.fillColor = { 0.64f, 0.25f, 0.18f, 0.78f };
                shapes3D.drawCube({ 1.2f, 0.5f, -1.0f }, 2.0f, cubeStyle);

                modelViewport.drawLabel3D(
                    text,
                    "Cube A",
                    { -1.7f, 1.15f, 0.0f },
                    17.0f,
                    { 0.84f, 0.94f, 1.0f, 1.0f });
                modelViewport.drawWorldText3D(
                    text,
                    "World-sized",
                    { 1.2f, 1.75f, -1.0f },
                    0.45f,
                    { 1.0f, 0.82f, 0.62f, 1.0f });
            }
        });

        app.Run(60);
    } catch (const std::exception& exception) {
        std::cerr << "WidgeCraft viewport sandbox failed: "
                  << exception.what() << '\n';
        return 1;
    }

    return 0;
}
