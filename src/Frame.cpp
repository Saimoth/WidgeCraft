#include "WidgeCraft/Frame.hpp"

#include "WidgeCraft/ShapeRenderer.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace WidgeCraft {

    namespace {
        void applyScissor(const Rect& clip) {
            const int x = static_cast<int>(std::floor(clip.x));
            const int y = static_cast<int>(std::floor(clip.y));
            const int right = static_cast<int>(std::ceil(clip.right()));
            const int top = static_cast<int>(std::ceil(clip.top()));
            glEnable(GL_SCISSOR_TEST);
            glScissor(x, y, std::max(0, right - x), std::max(0, top - y));
        }
    } // namespace

    Frame::Frame(std::string name, Frame* parent)
        : m_name(std::move(name))
        , m_parent(parent)
        , m_widgets(*this) {
        if (m_name.empty()) {
            throw std::invalid_argument("Frame names cannot be empty");
        }
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
        m_position = { x, y };
    }

    void Frame::setSize(float width, float height) {
        m_size = { std::max(0.0f, width), std::max(0.0f, height) };
    }

    Position Frame::getAbsolutePosition() const {
        if (!m_parent) {
            return m_position;
        }

        const Rect parentRect = m_parent->getAbsoluteRect();
        Position result{ parentRect.x, parentRect.y };
        switch (m_anchor) {
        case Anchor::TopLeft:
            result.x += m_position.x;
            result.y += parentRect.height - m_position.y - m_size.y;
            break;
        case Anchor::TopCenter:
            result.x += (parentRect.width - m_size.x) * 0.5f + m_position.x;
            result.y += parentRect.height - m_position.y - m_size.y;
            break;
        case Anchor::TopRight:
            result.x += parentRect.width - m_position.x - m_size.x;
            result.y += parentRect.height - m_position.y - m_size.y;
            break;
        case Anchor::CenterLeft:
            result.x += m_position.x;
            result.y += (parentRect.height - m_size.y) * 0.5f + m_position.y;
            break;
        case Anchor::Center:
            result.x += (parentRect.width - m_size.x) * 0.5f + m_position.x;
            result.y += (parentRect.height - m_size.y) * 0.5f + m_position.y;
            break;
        case Anchor::CenterRight:
            result.x += parentRect.width - m_position.x - m_size.x;
            result.y += (parentRect.height - m_size.y) * 0.5f + m_position.y;
            break;
        case Anchor::BottomLeft:
            result.x += m_position.x;
            result.y += m_position.y;
            break;
        case Anchor::BottomCenter:
            result.x += (parentRect.width - m_size.x) * 0.5f + m_position.x;
            result.y += m_position.y;
            break;
        case Anchor::BottomRight:
            result.x += parentRect.width - m_position.x - m_size.x;
            result.y += m_position.y;
            break;
        }
        return result;
    }

    Rect Frame::getAbsoluteRect() const {
        const Position position = getAbsolutePosition();
        return { position.x, position.y, m_size.x, m_size.y };
    }

    Frame& Frame::createChildFrame(const std::string& name) {
        if (findChildFrame(name)) {
            throw std::invalid_argument("A child frame named '" + name + "' already exists in frame '" + m_name + "'");
        }

        auto child = std::make_unique<Frame>(name, this);
        Frame& reference = *child;
        m_children.emplace_back(std::move(child));
        return reference;
    }

    Frame* Frame::findChildFrame(const std::string& name) {
        const auto iterator = std::find_if(m_children.begin(), m_children.end(), [&](const auto& frame) {
            return frame && frame->getName() == name;
        });
        return iterator != m_children.end() ? iterator->get() : nullptr;
    }

    const Frame* Frame::findChildFrame(const std::string& name) const {
        const auto iterator = std::find_if(m_children.begin(), m_children.end(), [&](const auto& frame) {
            return frame && frame->getName() == name;
        });
        return iterator != m_children.end() ? iterator->get() : nullptr;
    }

    bool Frame::removeChildFrame(const std::string& name) {
        const auto iterator = std::find_if(m_children.begin(), m_children.end(), [&](const auto& frame) {
            return frame && frame->getName() == name;
        });
        if (iterator == m_children.end() || !(*iterator)->canBeDeleted()) {
            return false;
        }
        m_children.erase(iterator);
        return true;
    }

    void Frame::render(TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) {
        const Rect rootClip = getAbsoluteRect();
        renderInternal(textRenderer, shapeRenderer, input, rootClip, true);
    }

    void Frame::renderInternal(
        TextRenderer& textRenderer,
        ShapeRenderer& shapeRenderer,
        const Input& input,
        const Rect& parentClip,
        bool isRoot) {

        if (!m_visible) {
            clearTextBatchesRecursive();
            return;
        }

        const Rect frameRect = getAbsoluteRect();
        const Rect activeClip = m_clipContents ? intersect(parentClip, frameRect) : parentClip;
        if (activeClip.width <= 0.0f || activeClip.height <= 0.0f) {
            clearTextBatchesRecursive();
            return;
        }

        shapeRenderer.flush();
        textRenderer.flush();
        applyScissor(activeClip);

        if (m_showBackground && frameRect.width > 0.0f && frameRect.height > 0.0f) {
            shapeRenderer.drawFilledRect(frameRect.x, frameRect.y, frameRect.width, frameRect.height, m_backgroundColor);
        }

        for (const auto& widget : m_widgets) {
            if (widget && widget->isVisible()) {
                widget->render(*this, textRenderer, shapeRenderer, input);
            }
        }

        for (const auto& command : m_textBatch.getCommands()) {
            const float size = command.sizePixels > 0.0f ? command.sizePixels : textRenderer.getBasePixelHeight();
            textRenderer.renderText(
                command.text,
                frameRect.x + command.position.x,
                frameRect.y + command.position.y,
                size,
                command.color);
        }
        m_textBatch.clear();

        if (m_showBorder && frameRect.width > 0.0f && frameRect.height > 0.0f && m_borderThickness > 0.0f) {
            shapeRenderer.drawRectOutline(
                frameRect.x,
                frameRect.y,
                frameRect.width,
                frameRect.height,
                m_borderThickness,
                m_borderColor);
        }

        // Shape geometry must land before glyphs at each retained UI layer.
        shapeRenderer.flush();
        textRenderer.flush();

        for (auto& child : m_children) {
            if (child) {
                child->renderInternal(textRenderer, shapeRenderer, input, activeClip, false);
            }
        }

        shapeRenderer.flush();
        textRenderer.flush();
        if (isRoot) {
            glDisable(GL_SCISSOR_TEST);
        } else {
            applyScissor(parentClip);
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

    void Frame::TextBatch::addText(std::string text, float x, float y, float sizePixels, Color color) {
        m_commands.push_back({ std::move(text), { x, y }, sizePixels, color });
    }

    void Frame::TextBatch::clear() {
        m_commands.clear();
    }

} // namespace WidgeCraft
