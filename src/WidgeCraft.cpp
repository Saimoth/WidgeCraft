#include "WidgeCraft/WidgeCraft.hpp"

#include "WidgeCraft/ShapeRenderer.hpp"
#include "WidgeCraft/TextRenderer.hpp"

// Order matters: glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

namespace WidgeCraft {

    namespace {
        std::string resolveDefaultFontPath() {
            const std::filesystem::path defaultPath = std::filesystem::path(WIDGECRAFT_ASSET_DIR) / "fonts/Roboto-Regular.ttf";
            return defaultPath.string();
        }
    }

    WidgeCraft::WidgeCraft(std::string title, int width, int height, std::string fontPath, float fontPixelHeight)
        : m_window(width, height, std::move(title))
        , m_textRenderer(m_window.getWidth(), m_window.getHeight(), resolveFontPath(fontPath), fontPixelHeight)
        , m_shapeRenderer() {
        initializeGraphics();

        m_rootFrameName = m_window.getRootFrame().getName();
        registerFrame(m_window.getRootFrame());
    }

    WidgeCraft::~WidgeCraft() {
        onExit();
    }

    void WidgeCraft::onStartup(StartupCallback callback) {
        m_startupCallback = std::move(callback);
        if (m_startupInvoked && m_startupCallback) {
            m_startupCallback(*this);
        }
    }

    void WidgeCraft::onUpdate(UpdateCallback callback) {
        m_updateCallback = std::move(callback);
    }

    void WidgeCraft::onExit(ExitCallback callback) {
        m_exitCallback = std::move(callback);
    }

    void WidgeCraft::onExit() {
        invokeExitIfNeeded();
    }

    void WidgeCraft::Run() {
        if (m_running) {
            return;
        }

        m_running = true;
        m_exitInvoked = false;

        invokeStartupIfNeeded();

        while (!shouldClose() && !m_exitInvoked) {
            updateInternal();
            renderInternal();

            glfwSwapBuffers(m_window.getNativeHandle());
            m_window.pollEvents();
        }

        m_running = false;
        invokeExitIfNeeded();
    }

    void WidgeCraft::Update() {
        invokeStartupIfNeeded();
        updateInternal();
        m_window.pollEvents();
    }

    void WidgeCraft::Render() {
        invokeStartupIfNeeded();
        renderInternal();
        glfwSwapBuffers(m_window.getNativeHandle());
    }

    Frame& WidgeCraft::addFrame(const std::string& frameName, const std::string& parentFrameName) {
        if (frameName.empty()) {
            throw std::invalid_argument("Frame name cannot be empty");
        }

        if (m_frameLookup.find(frameName) != m_frameLookup.end()) {
            throw std::invalid_argument("Frame already exists: " + frameName);
        }

        Frame& parent = resolveFrame(parentFrameName);
        Frame& child = parent.createChildFrame(frameName);
        registerFrame(child);
        return child;
    }

    bool WidgeCraft::removeFrame(const std::string& frameName) {
        if (frameName.empty() || frameName == m_rootFrameName) {
            return false;
        }

        auto it = m_frameLookup.find(frameName);
        if (it == m_frameLookup.end() || it->second == nullptr) {
            return false;
        }

        Frame* frame = it->second;
        if (!frame->canBeDeleted()) {
            return false;
        }

        Frame* parent = frame->getParent();
        if (!parent) {
            return false;
        }

        std::vector<std::string> framesToRemove;
        std::vector<Frame*> stack;
        stack.push_back(frame);
        while (!stack.empty()) {
            Frame* current = stack.back();
            stack.pop_back();
            if (!current) {
                continue;
            }

            framesToRemove.emplace_back(current->getName());
            for (const auto& child : current->getChildren()) {
                if (child) {
                    stack.push_back(child.get());
                }
            }
        }

        if (!parent->removeChildFrame(frameName)) {
            return false;
        }

        for (const auto& name : framesToRemove) {
            m_widgetLookup.erase(name);
            m_frameLookup.erase(name);
        }

        return true;
    }

    void WidgeCraft::frameSetVisible(const std::string& frameName, bool visible) {
        resolveFrame(frameName).setVisible(visible);
    }

    void WidgeCraft::frameSetBackgroundVisible(const std::string& frameName, bool visible) {
        resolveFrame(frameName).setBackgroundVisible(visible);
    }

    void WidgeCraft::frameSetBorderVisible(const std::string& frameName, bool visible) {
        resolveFrame(frameName).setBorderVisible(visible);
    }

    void WidgeCraft::frameSetBackgroundColor(const std::string& frameName, Color color) {
        resolveFrame(frameName).setBackgroundColor(color);
    }

    void WidgeCraft::frameSetPosition(const std::string& frameName, float x, float y) {
        resolveFrame(frameName).setPosition(x, y);
    }

    void WidgeCraft::frameSetSize(const std::string& frameName, float width, float height) {
        resolveFrame(frameName).setSize(width, height);
    }

    void WidgeCraft::frameSetAnchor(const std::string& frameName, Anchor anchor) {
        resolveFrame(frameName).setAnchor(anchor);
    }

    Position WidgeCraft::frameGetPosition(const std::string& frameName) const {
        return resolveFrame(frameName).getPosition();
    }

    Size WidgeCraft::frameGetSize(const std::string& frameName) const {
        return resolveFrame(frameName).getSize();
    }

    Label& WidgeCraft::addLabel(const std::string& frameName, const std::string& widgetName, std::string text) {
        return addWidgetImpl<Label>(frameName, widgetName, std::move(text));
    }

    Button& WidgeCraft::addButton(const std::string& frameName, const std::string& widgetName, std::string text) {
        return addWidgetImpl<Button>(frameName, widgetName, std::move(text));
    }

    bool WidgeCraft::removeWidget(const std::string& frameName, const std::string& widgetName) {
        Frame& frame = resolveFrame(frameName);
        auto mapIt = m_widgetLookup.find(frame.getName());
        if (mapIt == m_widgetLookup.end()) {
            return false;
        }

        auto widgetIt = mapIt->second.find(widgetName);
        if (widgetIt == mapIt->second.end() || widgetIt->second == nullptr) {
            return false;
        }

        Widget* widget = widgetIt->second;
        auto& widgets = frame.getWidgets().getAll();
        auto eraseIt = std::find_if(widgets.begin(), widgets.end(), [&](const std::unique_ptr<Widget>& candidate) {
            return candidate.get() == widget;
        });

        if (eraseIt == widgets.end()) {
            return false;
        }

        widgets.erase(eraseIt);
        mapIt->second.erase(widgetIt);
        return true;
    }

    void WidgeCraft::widgetSetVisible(const std::string& frameName, const std::string& widgetName, bool visible) {
        resolveWidget(frameName, widgetName).setVisible(visible);
    }

    void WidgeCraft::widgetSetPosition(const std::string& frameName, const std::string& widgetName, float x, float y) {
        resolveWidget(frameName, widgetName).setPosition(x, y);
    }

    void WidgeCraft::widgetSetSize(const std::string& frameName, const std::string& widgetName, float width, float height) {
        resolveWidget(frameName, widgetName).setSize(width, height);
    }

    void WidgeCraft::widgetSetText(const std::string& frameName, const std::string& widgetName, const std::string& text) {
        Widget& widget = resolveWidget(frameName, widgetName);
        if (auto* label = dynamic_cast<Label*>(&widget)) {
            label->setText(text);
            return;
        }
        if (auto* button = dynamic_cast<Button*>(&widget)) {
            button->setText(text);
            return;
        }

        throw std::invalid_argument("widgetSetText unsupported for widget: " + widgetName);
    }

    void WidgeCraft::widgetSetTextSize(const std::string& frameName, const std::string& widgetName, float sizePixels) {
        Widget& widget = resolveWidget(frameName, widgetName);
        if (auto* label = dynamic_cast<Label*>(&widget)) {
            label->setTextSize(sizePixels);
            return;
        }
        if (auto* button = dynamic_cast<Button*>(&widget)) {
            button->setTextSize(sizePixels);
            return;
        }

        throw std::invalid_argument("widgetSetTextSize unsupported for widget: " + widgetName);
    }

    void WidgeCraft::widgetSetColor(const std::string& frameName, const std::string& widgetName, TextRenderer::Color color) {
        Widget& widget = resolveWidget(frameName, widgetName);
        if (auto* label = dynamic_cast<Label*>(&widget)) {
            label->setColor(color);
            return;
        }
        if (auto* button = dynamic_cast<Button*>(&widget)) {
            button->setTextColor(color);
            return;
        }

        throw std::invalid_argument("widgetSetColor unsupported for widget: " + widgetName);
    }

    void WidgeCraft::widgetSetTextColor(const std::string& frameName, const std::string& widgetName, TextRenderer::Color color) {
        Widget& widget = resolveWidget(frameName, widgetName);
        if (auto* button = dynamic_cast<Button*>(&widget)) {
            button->setTextColor(color);
            return;
        }
        if (auto* label = dynamic_cast<Label*>(&widget)) {
            label->setColor(color);
            return;
        }

        throw std::invalid_argument("widgetSetTextColor unsupported for widget: " + widgetName);
    }

    void WidgeCraft::widgetSetBackgroundVisible(const std::string& frameName, const std::string& widgetName, bool visible) {
        Widget& widget = resolveWidget(frameName, widgetName);
        if (auto* button = dynamic_cast<Button*>(&widget)) {
            button->setBackgroundVisible(visible);
            return;
        }

        throw std::invalid_argument("widgetSetBackgroundVisible unsupported for widget: " + widgetName);
    }

    void WidgeCraft::widgetSetBackgroundColor(const std::string& frameName, const std::string& widgetName, Color color) {
        Widget& widget = resolveWidget(frameName, widgetName);
        if (auto* button = dynamic_cast<Button*>(&widget)) {
            button->setBackgroundColor(color);
            return;
        }

        throw std::invalid_argument("widgetSetBackgroundColor unsupported for widget: " + widgetName);
    }

    Position WidgeCraft::widgetGetPosition(const std::string& frameName, const std::string& widgetName) const {
        return resolveWidget(frameName, widgetName).getPosition();
    }

    Size WidgeCraft::widgetGetSize(const std::string& frameName, const std::string& widgetName) const {
        return resolveWidget(frameName, widgetName).getSize();
    }

    void WidgeCraft::initializeGraphics() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    std::string WidgeCraft::resolveFontPath(const std::string& fontPath) {
        if (!fontPath.empty()) {
            return fontPath;
        }

        return resolveDefaultFontPath();
    }

    void WidgeCraft::updateInternal() {
        m_textRenderer.setScreenSize(m_window.getWidth(), m_window.getHeight());
        m_shapeRenderer.setScreenSize(static_cast<float>(m_window.getWidth()), static_cast<float>(m_window.getHeight()));

        if (m_updateCallback) {
            m_updateCallback(*this);
        }
    }

    void WidgeCraft::renderInternal() {
        glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_renderCallback) {
            m_renderCallback(*this);
        }

        m_window.getRootFrame().render(m_textRenderer, m_shapeRenderer);
    }

    void WidgeCraft::invokeStartupIfNeeded() {
        if (m_startupInvoked) {
            return;
        }

        if (m_frameLookup.find(m_rootFrameName) == m_frameLookup.end()) {
            registerFrame(m_window.getRootFrame());
        }

        m_startupInvoked = true;

        if (m_startupCallback) {
            m_startupCallback(*this);
        }
    }

    void WidgeCraft::invokeExitIfNeeded() {
        if (m_exitInvoked) {
            return;
        }

        if (m_exitCallback) {
            m_exitCallback(*this);
        }

        m_exitInvoked = true;
        m_running = false;
        m_widgetLookup.clear();
        m_frameLookup.clear();
        m_startupInvoked = false;
    }

    Frame& WidgeCraft::resolveFrame(const std::string& frameName) {
        if (frameName.empty()) {
            return m_window.getRootFrame();
        }

        auto it = m_frameLookup.find(frameName);
        if (it == m_frameLookup.end() || it->second == nullptr) {
            throw std::invalid_argument("Unknown frame: " + frameName);
        }

        return *it->second;
    }

    const Frame& WidgeCraft::resolveFrame(const std::string& frameName) const {
        if (frameName.empty()) {
            return m_window.getRootFrame();
        }

        auto it = m_frameLookup.find(frameName);
        if (it == m_frameLookup.end() || it->second == nullptr) {
            throw std::invalid_argument("Unknown frame: " + frameName);
        }

        return *it->second;
    }

    Widget& WidgeCraft::resolveWidget(const std::string& frameName, const std::string& widgetName) {
        Frame& frame = resolveFrame(frameName);
        auto frameIt = m_widgetLookup.find(frame.getName());
        if (frameIt == m_widgetLookup.end()) {
            throw std::invalid_argument("Frame has no widget map: " + frame.getName());
        }

        auto widgetIt = frameIt->second.find(widgetName);
        if (widgetIt == frameIt->second.end() || widgetIt->second == nullptr) {
            throw std::invalid_argument("Unknown widget: " + widgetName + " in frame " + frame.getName());
        }

        return *widgetIt->second;
    }

    const Widget& WidgeCraft::resolveWidget(const std::string& frameName, const std::string& widgetName) const {
        const Frame& frame = resolveFrame(frameName);
        auto frameIt = m_widgetLookup.find(frame.getName());
        if (frameIt == m_widgetLookup.end()) {
            throw std::invalid_argument("Frame has no widget map: " + frame.getName());
        }

        auto widgetIt = frameIt->second.find(widgetName);
        if (widgetIt == frameIt->second.end() || widgetIt->second == nullptr) {
            throw std::invalid_argument("Unknown widget: " + widgetName + " in frame " + frame.getName());
        }

        return *widgetIt->second;
    }

    void WidgeCraft::registerFrame(Frame& frame) {
        m_frameLookup[frame.getName()] = &frame;
        m_widgetLookup.try_emplace(frame.getName());
    }

} // namespace WidgeCraft

