#pragma once

#include "WidgeCraft/Types.hpp"

#include <array>

namespace WidgeCraft {

    enum class MouseButton : int {
        Left = 0,
        Right = 1,
        Middle = 2
    };

    class Input {
    public:
        static constexpr int MaxKeys = 512;
        static constexpr int MaxMouseButtons = 8;

        void beginFrame();

        bool keyDown(int key) const;
        bool keyPressed(int key) const;
        bool keyReleased(int key) const;

        bool mouseDown(MouseButton button) const;
        bool mousePressed(MouseButton button) const;
        bool mouseReleased(MouseButton button) const;

        Vec2 mousePosition() const { return m_mousePosition; }
        Vec2 mouseDelta() const { return m_mouseDelta; }
        Vec2 scrollDelta() const { return m_scrollDelta; }

    private:
        friend class Window;

        void setKey(int key, bool down);
        void setMouseButton(int button, bool down);
        void setMousePosition(float x, float y);
        void addScroll(float x, float y);

        std::array<bool, MaxKeys> m_keys{};
        std::array<bool, MaxKeys> m_previousKeys{};
        std::array<bool, MaxMouseButtons> m_mouseButtons{};
        std::array<bool, MaxMouseButtons> m_previousMouseButtons{};
        Vec2 m_mousePosition{};
        Vec2 m_previousMousePosition{};
        Vec2 m_mouseDelta{};
        Vec2 m_scrollDelta{};
    };

} // namespace WidgeCraft
