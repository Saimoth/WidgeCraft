#include "WidgeCraft/Frame.hpp"

#include "WidgeCraft/ShapeRenderer.hpp"
#include "WidgeCraft/TextRenderer.hpp"

#include <algorithm>
#include <utility>

namespace WidgeCraft {
    Frame::Frame(std::string name, Frame* parent)
        : m_name(std::move(name)), m_parent(parent), m_widgets(*this) {
    }

    void Frame::setVisible(bool visible) {
        if (m_visible == visible) {
            return;
        }

        m_visible = visible;
        if (!m_visible) {
            clearTextBatchesRecursive();
        }
    }

    void Frame::setPosition(float x, float y) {
        m_position.x = x;
        m_position.y = y;
    }

    void Frame::setSize(float width, float height) {
        m_size.x = width;
        m_size.y = height;
    }

    Position Frame::getAbsolutePosition() const {
        if (!m_parent) {
            return m_position;
        }

        const Position parentPosition = m_parent->getAbsolutePosition();
        const Size parentSize = m_parent->getSize();
        Position result = parentPosition;

        switch (m_anchor) {
        case Anchor::TopLeft:
            result.x += m_position.x;
            result.y += parentSize.y - m_position.y - m_size.y;
            break;
        case Anchor::TopCenter:
            result.x += (parentSize.x - m_size.x) * 0.5f + m_position.x;
            result.y += parentSize.y - m_position.y - m_size.y;
            break;
        case Anchor::TopRight:
            result.x += parentSize.x - m_position.x - m_size.x;
            result.y += parentSize.y - m_position.y - m_size.y;
            break;
        case Anchor::CenterLeft:
            result.x += m_position.x;
            result.y += (parentSize.y - m_size.y) * 0.5f + m_position.y;
            break;
        case Anchor::Center:
            result.x += (parentSize.x - m_size.x) * 0.5f;
            result.y += (parentSize.y - m_size.y) * 0.5f;
            break;
        case Anchor::CenterRight:
            result.x += parentSize.x - m_position.x - m_size.x;
            result.y += (parentSize.y - m_size.y) * 0.5f + m_position.y;
            break;
        case Anchor::BottomLeft:
            result.x += m_position.x;
            result.y += m_position.y;
            break;
        case Anchor::BottomCenter:
            result.x += (parentSize.x - m_size.x) * 0.5f + m_position.x;
            result.y += m_position.y;
            break;
        case Anchor::BottomRight:
            result.x += parentSize.x - m_position.x - m_size.x;
            result.y += m_position.y;
            break;
        }

        return result;
    }

    Frame& Frame::createChildFrame(const std::string& name) {
        auto child = std::make_unique<Frame>(name, this);
        Frame& reference = *child;
        m_children.emplace_back(std::move(child));
        return reference;
    }

    bool Frame::removeChildFrame(const std::string& name) {
        auto it = std::find_if(m_children.begin(), m_children.end(), [&](const std::unique_ptr<Frame>& frame) {
            return frame && frame->getName() == name;
        });

        if (it == m_children.end() || !(*it)->canBeDeleted()) {
            return false;
        }

        m_children.erase(it);
        return true;
    }

    void Frame::render(TextRenderer& textRenderer, ShapeRenderer& shapeRenderer) {
        if (!m_visible) {
            clearTextBatchesRecursive();
            return;
        }

        const Position basePosition = getAbsolutePosition();
        if (m_showBackground && m_size.x > 0.0f && m_size.y > 0.0f) {
            shapeRenderer.drawFilledRect(basePosition.x, basePosition.y, m_size.x, m_size.y, m_backgroundColor);
        }

        for (const auto& widget : m_widgets) {
            if (widget && widget->isVisible()) {
                widget->render(*this, textRenderer, shapeRenderer);
            }
        }

        for (const auto& command : m_textBatch.getCommands()) {
            float size = command.sizePixels;
            if (size <= 0.0f) {
                size = textRenderer.getBasePixelHeight();
            }

            textRenderer.renderText(
                command.text,
                basePosition.x + command.position.x,
                basePosition.y + command.position.y,
                size,
                command.color);
        }

        m_textBatch.clear();

        for (auto& child : m_children) {
            if (child) {
                child->render(textRenderer, shapeRenderer);
            }
        }

        if (m_showBorder && m_size.x > 0.0f && m_size.y > 0.0f) {
            shapeRenderer.drawRectOutline(basePosition.x, basePosition.y, m_size.x, m_size.y, 2.0f, Colors::Black);
        }
    }

    void Frame::clearTextBatchesRecursive() {
        m_textBatch.clear();
        for (auto& child : m_children) {
            if (child) {
                child->clearTextBatchesRecursive();
            }
        }
    }

    Frame* Frame::getRoot() {
        Frame* current = this;
        while (current && current->m_parent) {
            current = current->m_parent;
        }
        return current;
    }

    const Frame* Frame::getRoot() const {
        const Frame* current = this;
        while (current && current->m_parent) {
            current = current->m_parent;
        }
        return current;
    }

    void Frame::TextBatch::addText(std::string text, float x, float y, float sizePixels, TextRenderer::Color color) {
        Command command;
        command.text = std::move(text);
        command.position.x = x;
        command.position.y = y;
        command.sizePixels = sizePixels;
        command.color = color;
        m_commands.emplace_back(std::move(command));
    }

    void Frame::TextBatch::clear() {
        m_commands.clear();
    }

} // namespace WidgeCraft
