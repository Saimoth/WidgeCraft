#pragma once

#include "WidgeCraft/ShapeRenderer.hpp"
#include "WidgeCraft/TextRenderer.hpp"
#include "WidgeCraft/Types.hpp"
#include "WidgeCraft/Widget.hpp"
#include "WidgeCraft/Window.hpp"

#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace WidgeCraft {

    class WidgeCraft {
    public:
        using StartupCallback = std::function<void(WidgeCraft&)>;
        using UpdateCallback = std::function<void(WidgeCraft&)>;
        using RenderCallback = std::function<void(WidgeCraft&)>;
        using ExitCallback = std::function<void(WidgeCraft&)>;

        WidgeCraft(std::string title, int width, int height, std::string fontPath = {}, float fontPixelHeight = 64.0f);
        ~WidgeCraft();

        WidgeCraft(const WidgeCraft&) = delete;
        WidgeCraft& operator=(const WidgeCraft&) = delete;
        WidgeCraft(WidgeCraft&&) noexcept = default;
        WidgeCraft& operator=(WidgeCraft&&) noexcept = default;

        void onStartup(StartupCallback callback);

        void onUpdate(UpdateCallback callback);

        void onRender(RenderCallback callback) { m_renderCallback = std::move(callback); }

        void onExit(ExitCallback callback);
        void onExit();

        void setStartupCallback(StartupCallback callback) { onStartup(std::move(callback)); }
        void setUpdateCallback(UpdateCallback callback) { onUpdate(std::move(callback)); }
        void setRenderCallback(RenderCallback callback) { onRender(std::move(callback)); }
        void setExitCallback(ExitCallback callback) { onExit(std::move(callback)); }

        void Run();
        void Update();
        void Render();

        void setClearColor(Color color) { m_clearColor = color; }
        Color getClearColor() const { return m_clearColor; }

        bool shouldClose() const { return m_window.shouldClose(); }

        Window& getWindow() { return m_window; }
        const Window& getWindow() const { return m_window; }

        Frame& getRootFrame() { return m_window.getRootFrame(); }
        const Frame& getRootFrame() const { return m_window.getRootFrame(); }

        const std::string& getRootFrameName() const { return m_rootFrameName; }

        TextRenderer& getTextRenderer() { return m_textRenderer; }
        const TextRenderer& getTextRenderer() const { return m_textRenderer; }

        ShapeRenderer& getShapeRenderer() { return m_shapeRenderer; }
        const ShapeRenderer& getShapeRenderer() const { return m_shapeRenderer; }

        Frame& addFrame(const std::string& frameName, const std::string& parentFrameName = {});
        bool removeFrame(const std::string& frameName);

        void frameSetVisible(const std::string& frameName, bool visible);
        void frameSetBackgroundVisible(const std::string& frameName, bool visible);
        void frameSetBorderVisible(const std::string& frameName, bool visible);
        void frameSetBackgroundColor(const std::string& frameName, Color color);
        void frameSetPosition(const std::string& frameName, float x, float y);
        void frameSetPosition(const std::string& frameName, const Position& position) { frameSetPosition(frameName, position.x, position.y); }
        void frameSetSize(const std::string& frameName, float width, float height);
        void frameSetSize(const std::string& frameName, const Size& size) { frameSetSize(frameName, size.x, size.y); }
        void frameSetAnchor(const std::string& frameName, Anchor anchor);

        Position frameGetPosition(const std::string& frameName) const;
        Size frameGetSize(const std::string& frameName) const;

        Label& addLabel(const std::string& frameName, const std::string& widgetName, std::string text = {});
        Button& addButton(const std::string& frameName, const std::string& widgetName, std::string text = {});
        bool removeWidget(const std::string& frameName, const std::string& widgetName);

        void widgetSetVisible(const std::string& frameName, const std::string& widgetName, bool visible);
        void widgetSetPosition(const std::string& frameName, const std::string& widgetName, float x, float y);
        void widgetSetPosition(const std::string& frameName, const std::string& widgetName, const Position& position) { widgetSetPosition(frameName, widgetName, position.x, position.y); }
        void widgetSetSize(const std::string& frameName, const std::string& widgetName, float width, float height);
        void widgetSetSize(const std::string& frameName, const std::string& widgetName, const Size& size) { widgetSetSize(frameName, widgetName, size.x, size.y); }
        void widgetSetText(const std::string& frameName, const std::string& widgetName, const std::string& text);
        void widgetSetTextSize(const std::string& frameName, const std::string& widgetName, float sizePixels);
        void widgetSetColor(const std::string& frameName, const std::string& widgetName, TextRenderer::Color color);
        void widgetSetTextColor(const std::string& frameName, const std::string& widgetName, TextRenderer::Color color);
        void widgetSetBackgroundVisible(const std::string& frameName, const std::string& widgetName, bool visible);
        void widgetSetBackgroundColor(const std::string& frameName, const std::string& widgetName, Color color);

        Position widgetGetPosition(const std::string& frameName, const std::string& widgetName) const;
        Size widgetGetSize(const std::string& frameName, const std::string& widgetName) const;

    private:
        void initializeGraphics();
        static std::string resolveFontPath(const std::string& fontPath);
        void updateInternal();
        void renderInternal();
        void invokeStartupIfNeeded();
        void invokeExitIfNeeded();

        Frame& resolveFrame(const std::string& frameName);
        const Frame& resolveFrame(const std::string& frameName) const;
        Widget& resolveWidget(const std::string& frameName, const std::string& widgetName);
        const Widget& resolveWidget(const std::string& frameName, const std::string& widgetName) const;
        void registerFrame(Frame& frame);

        template <typename WidgetType, typename... Args>
        WidgetType& addWidgetImpl(const std::string& frameName, const std::string& widgetName, Args&&... args);

        Window m_window;
        TextRenderer m_textRenderer;
        ShapeRenderer m_shapeRenderer;
        std::string m_rootFrameName;
        Color m_clearColor{ 0.2f, 0.3f, 0.3f, 1.0f };
        std::map<std::string, Frame*> m_frameLookup;
        std::map<std::string, std::map<std::string, Widget*>> m_widgetLookup;
        StartupCallback m_startupCallback;
        UpdateCallback m_updateCallback;
        RenderCallback m_renderCallback;
        ExitCallback m_exitCallback;
        bool m_startupInvoked = false;
        bool m_exitInvoked = false;
        bool m_running = false;
    };

} // namespace WidgeCraft

template <typename WidgetType, typename... Args>
WidgetType& WidgeCraft::addWidgetImpl(const std::string& frameName, const std::string& widgetName, Args&&... args) {
    Frame& frame = resolveFrame(frameName);
    auto& widgetMap = m_widgetLookup[frame.getName()];
    if (widgetMap.find(widgetName) != widgetMap.end()) {
        throw std::invalid_argument("Widget already exists: " + widgetName + " in frame " + frame.getName());
    }

    WidgetType& widget = frame.addWidget<WidgetType>(widgetName, std::forward<Args>(args)...);
    widgetMap[widgetName] = &widget;
    return widget;
}

