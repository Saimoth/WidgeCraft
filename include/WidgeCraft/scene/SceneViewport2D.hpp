#pragma once

#include "WidgeCraft/primitives/Types.hpp"

#include <algorithm>

namespace WidgeCraft {

    // Maps a fixed logical 2D scene into the current client area using one
    // uniform scale. Extra horizontal or vertical space is centred, so scene
    // geometry keeps its intended aspect ratio on every window shape.
    class SceneViewport2D {
    public:
        SceneViewport2D(float logicalWidth, float logicalHeight) {
            setLogicalSize(logicalWidth, logicalHeight);
        }

        void setLogicalSize(float width, float height) {
            m_logicalSize = {
                std::max(width, 1.0f),
                std::max(height, 1.0f)
            };
            resize(m_screenSize.x, m_screenSize.y);
        }

        void resize(float screenWidth, float screenHeight) {
            m_screenSize = {
                std::max(screenWidth, 0.0f),
                std::max(screenHeight, 0.0f)
            };

            if (m_screenSize.x <= 0.0f || m_screenSize.y <= 0.0f) {
                m_scale = 0.0f;
                m_offset = {};
                return;
            }

            m_scale = std::min(
                m_screenSize.x / m_logicalSize.x,
                m_screenSize.y / m_logicalSize.y);

            const Vec2 displayedSize = m_logicalSize * m_scale;
            m_offset = {
                (m_screenSize.x - displayedSize.x) * 0.5f,
                (m_screenSize.y - displayedSize.y) * 0.5f
            };
        }

        Vec2 toScreen(const Vec2& scenePosition) const {
            return m_offset + scenePosition * m_scale;
        }

        Vec2 toScene(const Vec2& screenPosition) const {
            if (m_scale <= 0.0f) {
                return {};
            }
            return (screenPosition - m_offset) / m_scale;
        }

        Rect toScreen(const Rect& sceneRect) const {
            const Vec2 bottomLeft = toScreen(
                Vec2{ sceneRect.x, sceneRect.y });
            return {
                bottomLeft.x,
                bottomLeft.y,
                sceneRect.width * m_scale,
                sceneRect.height * m_scale
            };
        }

        float scaleLength(float sceneLength) const {
            return sceneLength * m_scale;
        }

        Rect getScreenRect() const {
            return {
                m_offset.x,
                m_offset.y,
                m_logicalSize.x * m_scale,
                m_logicalSize.y * m_scale
            };
        }

        Vec2 getLogicalSize() const { return m_logicalSize; }
        Vec2 getScreenSize() const { return m_screenSize; }
        Vec2 getOffset() const { return m_offset; }
        float getScale() const { return m_scale; }

    private:
        Vec2 m_logicalSize{ 1.0f, 1.0f };
        Vec2 m_screenSize{};
        Vec2 m_offset{};
        float m_scale = 0.0f;
    };

} // namespace WidgeCraft
