#pragma once

#include "WidgeCraft/input/Input.hpp"
#include "WidgeCraft/model/Mesh.hpp"
#include "WidgeCraft/model/ModelLoader.hpp"
#include "WidgeCraft/physics/Collider.hpp"
#include "WidgeCraft/physics/PhysicsWorld.hpp"
#include "WidgeCraft/primitives/Shapes2D.hpp"
#include "WidgeCraft/primitives/Shapes3D.hpp"
#include "WidgeCraft/primitives/TextRenderer.hpp"
#include "WidgeCraft/primitives/Types.hpp"
#include "WidgeCraft/render/ShaderProgram.hpp"
#include "WidgeCraft/scene/Camera2D.hpp"
#include "WidgeCraft/scene/Camera3D.hpp"
#include "WidgeCraft/scene/ModelViewport.hpp"
#include "WidgeCraft/scene/ObjectManager.hpp"
#include "WidgeCraft/scene/Raycast.hpp"
#include "WidgeCraft/scene/Scene.hpp"
#include "WidgeCraft/scene/SceneManager.hpp"
#include "WidgeCraft/scene/SceneViewport2D.hpp"
#include "WidgeCraft/terrain/HeightMap.hpp"
#include "WidgeCraft/terrain/Terrain.hpp"
#include "WidgeCraft/ui/Frame.hpp"
#include "WidgeCraft/ui/UiManager.hpp"
#include "WidgeCraft/ui/UiScreen.hpp"
#include "WidgeCraft/ui/Widget.hpp"
#include "WidgeCraft/window/Window.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace WidgeCraft {

    struct ViewportRenderOptions {
        bool clearColor = false;
        bool clearDepth = true;
        Color color{ Colors::Black };
    };

    class WidgeCraft {
    public:
        using UpdateCallback = std::function<void(WidgeCraft&)>;
        using RenderCallback = std::function<void(WidgeCraft&)>;
        using ViewportRenderCallback = std::function<void(WidgeCraft&)>;

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
        Scene* getScene() { return m_sceneManager.getActive(); }
        const Scene* getScene() const { return m_sceneManager.getActive(); }

        SceneManager& getSceneManager() { return m_sceneManager; }
        const SceneManager& getSceneManager() const {
            return m_sceneManager;
        }
        UiManager& getUiManager() { return m_uiManager; }
        const UiManager& getUiManager() const { return m_uiManager; }

        // Selects a clipped model area. Selecting another viewport or clearing
        // this one flushes the queued geometry with the correct camera and clip.
        void useModelViewport(const ModelViewport& viewport);
        void clearModelViewport();

        // Runs and flushes one independently clipped render pass. Geometry and
        // scene text remain batched within the pass; multiple passes can safely
        // use different 2D/3D cameras in the same frame.
        void renderViewport(
            const ModelViewport& viewport,
            const ViewportRenderCallback& callback,
            const ViewportRenderOptions& options = {});
        bool hasActiveModelViewport() const {
            return m_modelViewportActive;
        }
        Rect getActiveModelViewportRect() const {
            return m_modelViewportRect;
        }

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
        void flushSceneQueues();
        void restoreFullFramebufferState();
        static std::string resolveFontPath(const std::string& fontPath);

        Window m_window;
        TextRenderer m_textRenderer;
        Shapes2D m_shapes2D;
        Shapes3D m_shapes3D;
        Color m_clearColor{ 0.055f, 0.070f, 0.095f, 1.0f };
        UpdateCallback m_updateCallback;
        RenderCallback m_renderCallback;
        SceneManager m_sceneManager;
        UiManager m_uiManager;
        Rect m_modelViewportRect{};
        ViewportRenderOptions m_modelViewportOptions{};
        bool m_modelViewportActive = false;
        bool m_viewportCallbackActive = false;
        float m_deltaTime = 0.0f;
        double m_elapsedTime = 0.0;
        int m_targetFrameRate = 60;
    };

} // namespace WidgeCraft
