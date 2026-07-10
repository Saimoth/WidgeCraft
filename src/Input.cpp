#include "WidgeCraft/Input.hpp"

#include <algorithm>

namespace WidgeCraft {

    namespace {
        bool validIndex(int index, int maximum) {
            return index >= 0 && index < maximum;
        }
    }

    void Input::beginFrame() {
        m_previousKeys = m_keys;
        m_previousMouseButtons = m_mouseButtons;
        m_previousMousePosition = m_mousePosition;
        m_mouseDelta = {};
        m_scrollDelta = {};
    }

    bool Input::keyDown(int key) const {
        return validIndex(key, MaxKeys) && m_keys[static_cast<std::size_t>(key)];
    }

    bool Input::keyPressed(int key) const {
        return validIndex(key, MaxKeys)
            && m_keys[static_cast<std::size_t>(key)]
            && !m_previousKeys[static_cast<std::size_t>(key)];
    }

    bool Input::keyReleased(int key) const {
        return validIndex(key, MaxKeys)
            && !m_keys[static_cast<std::size_t>(key)]
            && m_previousKeys[static_cast<std::size_t>(key)];
    }

    bool Input::mouseDown(MouseButton button) const {
        const int index = static_cast<int>(button);
        return validIndex(index, MaxMouseButtons) && m_mouseButtons[static_cast<std::size_t>(index)];
    }

    bool Input::mousePressed(MouseButton button) const {
        const int index = static_cast<int>(button);
        return validIndex(index, MaxMouseButtons)
            && m_mouseButtons[static_cast<std::size_t>(index)]
            && !m_previousMouseButtons[static_cast<std::size_t>(index)];
    }

    bool Input::mouseReleased(MouseButton button) const {
        const int index = static_cast<int>(button);
        return validIndex(index, MaxMouseButtons)
            && !m_mouseButtons[static_cast<std::size_t>(index)]
            && m_previousMouseButtons[static_cast<std::size_t>(index)];
    }

    void Input::setKey(int key, bool down) {
        if (validIndex(key, MaxKeys)) {
            m_keys[static_cast<std::size_t>(key)] = down;
        }
    }

    void Input::setMouseButton(int button, bool down) {
        if (validIndex(button, MaxMouseButtons)) {
            m_mouseButtons[static_cast<std::size_t>(button)] = down;
        }
    }

    void Input::setMousePosition(float x, float y) {
        const Vec2 next{ x, y };
        m_mouseDelta += next - m_mousePosition;
        m_mousePosition = next;
    }

    void Input::addScroll(float x, float y) {
        m_scrollDelta += Vec2{ x, y };
    }

} // namespace WidgeCraft
