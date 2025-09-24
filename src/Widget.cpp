#include "WidgeCraft/Widget.hpp"

#include "WidgeCraft/Frame.hpp"
#include "WidgeCraft/ShapeRenderer.hpp"
#include "WidgeCraft/TextRenderer.hpp"

#include <algorithm>
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
        m_hasExplicitSize = true;
    }

    void Widget::setParent(Frame* parent) {
        m_parent = parent;
    }

    void Widget::setComputedSize(float width, float height) {
        m_size.x = width;
        m_size.y = height;
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

    void Label::render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer) {
        float size = m_textSizePixels;
        if (size <= 0.0f) {
            size = textRenderer.getBasePixelHeight();
        }

        const Position position = getPosition();
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float boundsWidth = 0.0f;
        float boundsHeight = 0.0f;
        bool haveBounds = false;

        if (!m_text.empty()) {
            if (auto bounds = textRenderer.measureTextBounds(m_text, size)) {
                boundsWidth = bounds->maxX - bounds->minX;
                boundsHeight = bounds->maxY - bounds->minY;
                offsetX = bounds->minX;
                offsetY = bounds->minY;
                haveBounds = true;
            }
        }

        float finalWidth = boundsWidth;
        float finalHeight = boundsHeight;

        if (hasExplicitSize()) {
            const Size explicitSize = getSize();
            if (explicitSize.x > 0.0f) {
                finalWidth = std::max(finalWidth, explicitSize.x);
            }
            if (explicitSize.y > 0.0f) {
                finalHeight = std::max(finalHeight, explicitSize.y);
            }
        } else {
            if (finalWidth <= 0.0f) {
                finalWidth = size;
            }
            if (finalHeight <= 0.0f) {
                finalHeight = size;
            }
        }

        const Position framePosition = frame.getAbsolutePosition();
        const float borderX = framePosition.x + position.x + (haveBounds ? offsetX : 0.0f);
        const float borderY = framePosition.y + position.y + (haveBounds ? offsetY : 0.0f);
        shapeRenderer.drawRectOutline(borderX, borderY, finalWidth, finalHeight, 1.0f, Colors::Black);
        setComputedSize(finalWidth, finalHeight);

        if (!m_text.empty()) {
            frame.getTextBatch().addText(m_text, position.x, position.y, size, m_color);
        }
    }

    Button::Button(std::string name, std::string text)
        : Widget(std::move(name)), m_text(std::move(text)) {
    }

    void Button::setText(std::string text) {
        m_text = std::move(text);
    }

    void Button::render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer) {
        float size = m_textSizePixels;
        if (size <= 0.0f) {
            size = textRenderer.getBasePixelHeight();
        }

        const Position position = getPosition();
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float boundsWidth = 0.0f;
        float boundsHeight = 0.0f;
        bool haveBounds = false;

        if (!m_text.empty()) {
            if (auto bounds = textRenderer.measureTextBounds(m_text, size)) {
                boundsWidth = bounds->maxX - bounds->minX;
                boundsHeight = bounds->maxY - bounds->minY;
                offsetX = bounds->minX;
                offsetY = bounds->minY;
                haveBounds = true;
            }
        }

        float finalWidth = boundsWidth;
        float finalHeight = boundsHeight;

        if (hasExplicitSize()) {
            const Size explicitSize = getSize();
            if (explicitSize.x > 0.0f) {
                finalWidth = std::max(finalWidth, explicitSize.x);
            }
            if (explicitSize.y > 0.0f) {
                finalHeight = std::max(finalHeight, explicitSize.y);
            }
        } else {
            if (finalWidth <= 0.0f) {
                finalWidth = size;
            }
            if (finalHeight <= 0.0f) {
                finalHeight = size;
            }
        }

        const Position framePosition = frame.getAbsolutePosition();
        const float rectX = framePosition.x + position.x + (haveBounds ? offsetX : 0.0f);
        const float rectY = framePosition.y + position.y + (haveBounds ? offsetY : 0.0f);

        if (m_showBackground) {
            shapeRenderer.drawFilledRect(rectX, rectY, finalWidth, finalHeight, m_backgroundColor);
        }

        shapeRenderer.drawRectOutline(rectX, rectY, finalWidth, finalHeight, 1.0f, Colors::Black);
        setComputedSize(finalWidth, finalHeight);

        if (!m_text.empty()) {
            frame.getTextBatch().addText(m_text, position.x, position.y, size, m_textColor);
        }
    }

} // namespace WidgeCraft

