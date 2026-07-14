#pragma once

#include "WidgeCraft/primitives/Types.hpp"
#include "WidgeCraft/scene/ModelViewport.hpp"
#include "WidgeCraft/ui/Frame.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WidgeCraft {

    class WidgeCraft;

    // A scene render pass whose rectangle follows a retained UI frame. The
    // frame itself is still rendered afterwards, so its border and widgets
    // naturally overlay the scene content.
    class SceneView {
    public:
        using RenderCallback =
            std::function<void(WidgeCraft&, const SceneView&)>;

        SceneView(std::string name, Frame& frame);

        const std::string& getName() const { return m_name; }
        Frame& getFrame() { return *m_frame; }
        const Frame& getFrame() const { return *m_frame; }

        ModelViewport& getViewport() { return m_viewport; }
        const ModelViewport& getViewport() const { return m_viewport; }

        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        void setInset(float inset);
        void setInsets(float left, float bottom, float right, float top);

        void setClearColor(Color color) {
            m_clearColor = color;
            m_clearsColor = true;
        }
        void setClearColorEnabled(bool enabled) { m_clearsColor = enabled; }
        bool clearsColor() const { return m_clearsColor; }
        Color getClearColor() const { return m_clearColor; }

        void setRenderCallback(RenderCallback callback) {
            m_renderCallback = std::move(callback);
        }

    private:
        friend class UiScreen;

        void render(WidgeCraft& app);

        std::string m_name;
        Frame* m_frame = nullptr;
        ModelViewport m_viewport;
        RenderCallback m_renderCallback;
        Color m_clearColor{ 0.025f, 0.035f, 0.055f, 1.0f };
        float m_leftInset = 0.0f;
        float m_bottomInset = 0.0f;
        float m_rightInset = 0.0f;
        float m_topInset = 0.0f;
        bool m_visible = true;
        bool m_clearsColor = true;
    };

    class UiScreen {
    public:
        explicit UiScreen(std::string name);
        virtual ~UiScreen() = default;

        UiScreen(const UiScreen&) = delete;
        UiScreen& operator=(const UiScreen&) = delete;

        const std::string& getName() const { return m_name; }
        Frame& getRootFrame() { return m_rootFrame; }
        const Frame& getRootFrame() const { return m_rootFrame; }

        SceneView& createSceneView(
            const std::string& name,
            Frame& frame);
        SceneView* findSceneView(const std::string& name);
        const SceneView* findSceneView(const std::string& name) const;
        bool removeSceneView(const std::string& name);

        virtual void onAttach(WidgeCraft& app) { (void)app; }
        virtual void onDetach(WidgeCraft& app) { (void)app; }
        virtual void onUpdate(WidgeCraft& app, float deltaTime) {
            (void)app;
            (void)deltaTime;
        }
        virtual void onRender(WidgeCraft& app) { (void)app; }

    private:
        friend class UiManager;

        void setScreenSize(float width, float height);
        void renderSceneViews(WidgeCraft& app);

        std::string m_name;
        Frame m_rootFrame;
        std::vector<std::unique_ptr<SceneView>> m_sceneViews;
    };

} // namespace WidgeCraft
