#pragma once

#include "WidgeCraft/primitives/Types.hpp"

#include <algorithm>

namespace WidgeCraft {

    class Camera2D {
    public:
        void setPosition(const Vec2& position) { m_position = position; }
        Vec2 getPosition() const { return m_position; }

        void pan(const Vec2& amount) { m_position += amount; }

        void setZoom(float pixelsPerWorldUnit) {
            m_zoom = std::max(pixelsPerWorldUnit, 0.0001f);
        }
        float getZoom() const { return m_zoom; }

        Vec2 worldToScreen(const Vec2& worldPosition, const Rect& viewport) const {
            const Vec2 center{
                viewport.x + viewport.width * 0.5f,
                viewport.y + viewport.height * 0.5f
            };
            return center + (worldPosition - m_position) * m_zoom;
        }

        Vec2 screenToWorld(const Vec2& screenPosition, const Rect& viewport) const {
            const Vec2 center{
                viewport.x + viewport.width * 0.5f,
                viewport.y + viewport.height * 0.5f
            };
            return m_position + (screenPosition - center) / m_zoom;
        }

        Vec2 getRenderOffset(const Rect& viewport) const {
            const Vec2 center{
                viewport.x + viewport.width * 0.5f,
                viewport.y + viewport.height * 0.5f
            };
            return center - m_position * m_zoom;
        }

    private:
        Vec2 m_position{};
        float m_zoom = 1.0f;
    };

} // namespace WidgeCraft
