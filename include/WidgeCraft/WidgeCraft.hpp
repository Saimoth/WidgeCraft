#pragma once

#include "WidgeCraft/input/Input.hpp"
#include "WidgeCraft/primitives/Shapes2D.hpp"
#include "WidgeCraft/primitives/Shapes3D.hpp"
#include "WidgeCraft/primitives/TextRenderer.hpp"
#include "WidgeCraft/primitives/Types.hpp"
#include "WidgeCraft/render/ShaderProgram.hpp"
#include "WidgeCraft/scene/Raycast.hpp"
#include "WidgeCraft/scene/Scene.hpp"
#include "WidgeCraft/scene/SceneViewport2D.hpp"
#include "WidgeCraft/ui/Frame.hpp"
#include "WidgeCraft/ui/Widget.hpp"
#include "WidgeCraft/window/Window.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace WidgeCraft {

    class WidgeCraft {
    public:
        using UpdateCallback = std::function<void(WidgeCraft&)>;
        using RenderCallback = std::function<void(WidgeCraft&)>;

        WidgeCraft(
            std::string title,
            int width,
            int height,
            std::string fontPath = {},
            float fontPixelHeight = 128.0f);
        ~WidgeCraft();

        WidgeCraft(const WidgeCraft&) = delete;
        WidgeCraft& operator=(const WidgeCraft&) = delete;
        WidgeCraft(WidgeCraft&&) = delete;
        WidgeCraft& operator=(WidgeCraft&&) = delete;

        void Run(int targetFramesPerSecond = 60);
        void Stop();
        void Update();
        void Render();

        void setUpdateCallback(UpdateCallback callback) {
            m_updateCallback = std::move(callback);
        }
        void setRenderCallback(RenderCallback callback) {
            m_renderCallback = std::move(callback);
        }

        void setScene(std::unique_ptr<Scene> scene);
        Scene* getScene() { return m_scene.get(); }
        const Scene* getScene() const { return m_scene.get(); }

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

        Shapes2D& getShapes2D() { return m_shapes2D; }
        const Shapes2D& getShapes2D() const { return m_shapes2D; }
        Shapes3D& getShapes3D() { return m_shapes3D; }
        const Shapes3D& getShapes3D() const { return m_shapes3D; }

        // Compatibility with the original API while callers move to getShapes2D().
        ShapeRenderer& getShapeRenderer() { return m_shapes2D; }
        const ShapeRenderer& getShapeRenderer() const { return m_shapes2D; }

    private:
        void initializeGraphics();
        static std::string resolveFontPath(const std::string& fontPath);

        Window m_window;
        TextRenderer m_textRenderer;
        Shapes2D m_shapes2D;
        Shapes3D m_shapes3D;
        Color m_clearColor{ 0.055f, 0.070f, 0.095f, 1.0f };
        UpdateCallback m_updateCallback;
        RenderCallback m_renderCallback;
        std::unique_ptr<Scene> m_scene;
        float m_deltaTime = 0.0f;
        double m_elapsedTime = 0.0;
        int m_targetFrameRate = 60;
    };

} // namespace WidgeCraft
