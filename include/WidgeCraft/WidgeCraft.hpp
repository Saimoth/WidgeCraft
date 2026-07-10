#pragma once

#include "WidgeCraft/Input.hpp"
#include "WidgeCraft/Raycast.hpp"
#include "WidgeCraft/ShapeRenderer.hpp"
#include "WidgeCraft/TextRenderer.hpp"
#include "WidgeCraft/Types.hpp"
#include "WidgeCraft/Window.hpp"

#include <functional>
#include <string>

namespace WidgeCraft {

    class WidgeCraft {
    public:
        using UpdateCallback = std::function<void(WidgeCraft&)>;
        using RenderCallback = std::function<void(WidgeCraft&)>;

        WidgeCraft(std::string title, int width, int height, std::string fontPath = {}, float fontPixelHeight = 64.0f);
        ~WidgeCraft() = default;

        WidgeCraft(const WidgeCraft&) = delete;
        WidgeCraft& operator=(const WidgeCraft&) = delete;
        WidgeCraft(WidgeCraft&&) = delete;
        WidgeCraft& operator=(WidgeCraft&&) = delete;

        void Run(int targetFramesPerSecond = 60);
        void Stop();
        void Update();
        void Render();

        void setUpdateCallback(UpdateCallback callback) { m_updateCallback = std::move(callback); }
        void setRenderCallback(RenderCallback callback) { m_renderCallback = std::move(callback); }

        void setClearColor(Color color) { m_clearColor = color; }
        Color getClearColor() const { return m_clearColor; }

        float getDeltaTime() const { return m_deltaTime; }
        double getElapsedTime() const { return m_elapsedTime; }
        int getTargetFrameRate() const { return m_targetFrameRate; }

        Window& getWindow() { return m_window; }
        const Window& getWindow() const { return m_window; }
        Input& getInput() { return m_window.getInput(); }
        const Input& getInput() const { return m_window.getInput(); }

        Frame& getRootFrame() { return m_window.getRootFrame(); }
        const Frame& getRootFrame() const { return m_window.getRootFrame(); }

        TextRenderer& getTextRenderer() { return m_textRenderer; }
        const TextRenderer& getTextRenderer() const { return m_textRenderer; }

        ShapeRenderer& getShapeRenderer() { return m_shapeRenderer; }
        const ShapeRenderer& getShapeRenderer() const { return m_shapeRenderer; }

    private:
        void initializeGraphics();
        static std::string resolveFontPath(const std::string& fontPath);

        Window m_window;
        TextRenderer m_textRenderer;
        ShapeRenderer m_shapeRenderer;
        Color m_clearColor{ 0.055f, 0.070f, 0.095f, 1.0f };
        UpdateCallback m_updateCallback;
        RenderCallback m_renderCallback;
        float m_deltaTime = 0.0f;
        double m_elapsedTime = 0.0;
        int m_targetFrameRate = 60;
    };

} // namespace WidgeCraft
