#include "WidgeCraft/ui/UiScreen.hpp"

#include "WidgeCraft/WidgeCraft.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace WidgeCraft {

    SceneView::SceneView(std::string name, Frame& frame)
        : m_name(std::move(name))
        , m_frame(&frame) {
        if (m_name.empty()) {
            throw std::invalid_argument("Scene view names cannot be empty");
        }
    }

    void SceneView::setInset(float inset) {
        const float sanitized = std::max(0.0f, inset);
        setInsets(sanitized, sanitized, sanitized, sanitized);
    }

    void SceneView::setInsets(
        float left,
        float bottom,
        float right,
        float top) {

        m_leftInset = std::max(0.0f, left);
        m_bottomInset = std::max(0.0f, bottom);
        m_rightInset = std::max(0.0f, right);
        m_topInset = std::max(0.0f, top);
    }

    void SceneView::render(WidgeCraft& app) {
        if (!m_visible || !m_renderCallback || !m_frame
            || !m_frame->isVisibleInHierarchy()) {
            return;
        }

        const Rect frameRect = m_frame->getAbsoluteRect();
        const Rect insetRect{
            frameRect.x + m_leftInset,
            frameRect.y + m_bottomInset,
            std::max(
                frameRect.width - m_leftInset - m_rightInset,
                0.0f),
            std::max(
                frameRect.height - m_bottomInset - m_topInset,
                0.0f)
        };
        const Rect viewportRect = intersect(
            insetRect,
            m_frame->getVisibleRect());
        if (viewportRect.width <= 0.0f || viewportRect.height <= 0.0f) {
            return;
        }

        m_viewport.setScreenRect(viewportRect);
        ViewportRenderOptions options;
        options.clearColor = m_clearsColor;
        options.color = m_clearColor;
        options.clearDepth = true;

        app.renderViewport(
            m_viewport,
            [this](WidgeCraft& engine) {
                m_renderCallback(engine, *this);
            },
            options);
    }

    UiScreen::UiScreen(std::string name)
        : m_name(std::move(name))
        , m_rootFrame(m_name + " Root") {
        if (m_name.empty()) {
            throw std::invalid_argument("UI screen names cannot be empty");
        }
        m_rootFrame.setDeletable(false);
        m_rootFrame.setAnchor(Anchor::BottomLeft);
        m_rootFrame.setPosition(0.0f, 0.0f);
        m_rootFrame.setBackgroundVisible(false);
        m_rootFrame.setBorderVisible(false);
        m_rootFrame.setClipContents(true);
    }

    SceneView& UiScreen::createSceneView(
        const std::string& name,
        Frame& frame) {

        if (findSceneView(name)) {
            throw std::invalid_argument(
                "A scene view named '" + name
                + "' already exists in UI screen '" + m_name + "'");
        }

        auto view = std::make_unique<SceneView>(name, frame);
        SceneView& reference = *view;
        m_sceneViews.emplace_back(std::move(view));
        return reference;
    }

    SceneView* UiScreen::findSceneView(const std::string& name) {
        const auto iterator = std::find_if(
            m_sceneViews.begin(),
            m_sceneViews.end(),
            [&](const auto& view) {
                return view && view->getName() == name;
            });
        return iterator != m_sceneViews.end() ? iterator->get() : nullptr;
    }

    const SceneView* UiScreen::findSceneView(
        const std::string& name) const {

        const auto iterator = std::find_if(
            m_sceneViews.begin(),
            m_sceneViews.end(),
            [&](const auto& view) {
                return view && view->getName() == name;
            });
        return iterator != m_sceneViews.end() ? iterator->get() : nullptr;
    }

    bool UiScreen::removeSceneView(const std::string& name) {
        const auto iterator = std::find_if(
            m_sceneViews.begin(),
            m_sceneViews.end(),
            [&](const auto& view) {
                return view && view->getName() == name;
            });
        if (iterator == m_sceneViews.end()) {
            return false;
        }
        m_sceneViews.erase(iterator);
        return true;
    }

    void UiScreen::setScreenSize(float width, float height) {
        m_rootFrame.setSize(
            std::max(width, 0.0f),
            std::max(height, 0.0f));
    }

    void UiScreen::renderSceneViews(WidgeCraft& app) {
        for (auto& view : m_sceneViews) {
            if (view) {
                view->render(app);
            }
        }
    }

} // namespace WidgeCraft
