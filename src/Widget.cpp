#include "WidgeCraft/Widget.hpp"

#include "WidgeCraft/Frame.hpp"
#include "WidgeCraft/TextRenderer.hpp"

#include <utility>

namespace WidgeCraft {

    Widget::Widget(std::string name)
        : m_name(std::move(name)) {
    }

    void Widget::setVisible(bool visible) {
        m_visible = visible;
    }

    void Widget::setPosition(float x, float y) {
        m_position.x = x;
        m_position.y = y;
    }

    void Widget::setSize(float width, float height) {
        m_size.x = width;
        m_size.y = height;
    }

    void Widget::setParent(Frame* parent) {
        m_parent = parent;
    }

    Widgets::Widgets(Frame& owner)
        : m_owner(owner) {
    }

    Label::Label(std::string name, std::string text)
        : Widget(std::move(name)), m_text(std::move(text)) {
    }

    void Label::setText(std::string text) {
        m_text = std::move(text);
    }

    void Label::render(Frame& frame, TextRenderer& textRenderer) {
        if (m_text.empty()) {
            return;
        }

        float size = m_textSizePixels;
        if (size <= 0.0f) {
            size = textRenderer.getBasePixelHeight();
        }

        const Position position = getPosition();
        frame.getTextBatch().addText(m_text, position.x, position.y, size, m_color);
    }

    Button::Button(std::string name, std::string text)
        : Widget(std::move(name)), m_text(std::move(text)) {
    }

    void Button::setText(std::string text) {
        m_text = std::move(text);
    }

    void Button::render(Frame& frame, TextRenderer& textRenderer) {
        if (m_text.empty()) {
            return;
        }

        float size = m_textSizePixels;
        if (size <= 0.0f) {
            size = textRenderer.getBasePixelHeight();
        }

        const Position position = getPosition();
        frame.getTextBatch().addText(m_text, position.x, position.y, size, m_textColor);
    }

} // namespace WidgeCraft

