#include "WidgeCraft/Widget.hpp"

#include "WidgeCraft/Frame.hpp"
#include "WidgeCraft/ShapeRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace WidgeCraft {

    namespace {
        float defaultUiTextSize(const TextRenderer& textRenderer) {
            return std::max(14.0f, textRenderer.getBasePixelHeight() * 0.375f);
        }

        float alignmentOffset(HorizontalAlignment alignment, float availableWidth) {
            switch (alignment) {
            case HorizontalAlignment::Center:
                return availableWidth * 0.5f;
            case HorizontalAlignment::Right:
                return availableWidth;
            case HorizontalAlignment::Left:
            default:
                return 0.0f;
            }
        }
    } // namespace

    Widget::Widget(std::string name)
        : m_name(std::move(name)) {
        if (m_name.empty()) {
            throw std::invalid_argument("Widget names cannot be empty");
        }
    }

    void Widget::setPosition(float x, float y) {
        m_position = { x, y };
    }

    void Widget::setSize(float width, float height) {
        m_size = { std::max(0.0f, width), std::max(0.0f, height) };
        m_hasExplicitSize = true;
    }

    void Widget::setParent(Frame* parent) {
        m_parent = parent;
    }

    void Widget::setComputedSize(float width, float height) {
        m_size = { std::max(0.0f, width), std::max(0.0f, height) };
    }

    Rect Widget::resolveRect(const Frame& frame, const Size& desiredSize) const {
        const Rect parentRect = frame.getAbsoluteRect();
        Size size = desiredSize;
        if (m_hasExplicitSize) {
            if (m_size.x > 0.0f) {
                size.x = m_size.x;
            }
            if (m_size.y > 0.0f) {
                size.y = m_size.y;
            }
        }

        Rect result{ parentRect.x, parentRect.y, std::max(0.0f, size.x), std::max(0.0f, size.y) };
        switch (m_anchor) {
        case Anchor::TopLeft:
            result.x += m_position.x;
            result.y += parentRect.height - m_position.y - result.height;
            break;
        case Anchor::TopCenter:
            result.x += (parentRect.width - result.width) * 0.5f + m_position.x;
            result.y += parentRect.height - m_position.y - result.height;
            break;
        case Anchor::TopRight:
            result.x += parentRect.width - m_position.x - result.width;
            result.y += parentRect.height - m_position.y - result.height;
            break;
        case Anchor::CenterLeft:
            result.x += m_position.x;
            result.y += (parentRect.height - result.height) * 0.5f + m_position.y;
            break;
        case Anchor::Center:
            result.x += (parentRect.width - result.width) * 0.5f + m_position.x;
            result.y += (parentRect.height - result.height) * 0.5f + m_position.y;
            break;
        case Anchor::CenterRight:
            result.x += parentRect.width - m_position.x - result.width;
            result.y += (parentRect.height - result.height) * 0.5f + m_position.y;
            break;
        case Anchor::BottomLeft:
            result.x += m_position.x;
            result.y += m_position.y;
            break;
        case Anchor::BottomCenter:
            result.x += (parentRect.width - result.width) * 0.5f + m_position.x;
            result.y += m_position.y;
            break;
        case Anchor::BottomRight:
            result.x += parentRect.width - m_position.x - result.width;
            result.y += m_position.y;
            break;
        }
        return result;
    }

    Rect Widget::getAbsoluteRect() const {
        if (!m_parent) {
            return { m_position.x, m_position.y, m_size.x, m_size.y };
        }
        return resolveRect(*m_parent, m_size);
    }

    Widgets::Widgets(Frame& owner)
        : m_owner(owner) {
    }

    Widget* Widgets::find(const std::string& name) {
        const auto iterator = std::find_if(m_widgets.begin(), m_widgets.end(), [&](const auto& widget) {
            return widget && widget->getName() == name;
        });
        return iterator != m_widgets.end() ? iterator->get() : nullptr;
    }

    const Widget* Widgets::find(const std::string& name) const {
        const auto iterator = std::find_if(m_widgets.begin(), m_widgets.end(), [&](const auto& widget) {
            return widget && widget->getName() == name;
        });
        return iterator != m_widgets.end() ? iterator->get() : nullptr;
    }

    bool Widgets::remove(const std::string& name) {
        const auto iterator = std::find_if(m_widgets.begin(), m_widgets.end(), [&](const auto& widget) {
            return widget && widget->getName() == name;
        });
        if (iterator == m_widgets.end()) {
            return false;
        }
        m_widgets.erase(iterator);
        return true;
    }

    Label::Label(std::string name, std::string text)
        : Widget(std::move(name))
        , m_text(std::move(text)) {
    }

    void Label::render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) {
        (void)shapeRenderer;
        (void)input;

        const float textSize = m_textSizePixels > 0.0f ? m_textSizePixels : defaultUiTextSize(textRenderer);
        const auto bounds = textRenderer.measureTextBounds(m_text, textSize);
        const Size desiredSize = bounds ? Size{ bounds->width(), bounds->height() } : Size{};
        const Rect rect = resolveRect(frame, desiredSize);
        setComputedSize(rect.width, rect.height);

        if (!bounds || m_text.empty()) {
            return;
        }

        const float availableWidth = std::max(0.0f, rect.width - bounds->width());
        const float baselineX = rect.x - bounds->minX + alignmentOffset(m_alignment, availableWidth);
        const float baselineY = rect.y + (rect.height - bounds->height()) * 0.5f - bounds->minY;
        textRenderer.renderText(m_text, baselineX, baselineY, textSize, m_color);
    }

    Button::Button(std::string name, std::string text)
        : Widget(std::move(name))
        , m_text(std::move(text)) {
    }

    void Button::setPadding(float horizontal, float vertical) {
        m_horizontalPadding = std::max(0.0f, horizontal);
        m_verticalPadding = std::max(0.0f, vertical);
    }

    void Button::render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) {
        const float textSize = m_textSizePixels > 0.0f ? m_textSizePixels : defaultUiTextSize(textRenderer);
        const auto bounds = textRenderer.measureTextBounds(m_text, textSize);
        const float textWidth = bounds ? bounds->width() : 0.0f;
        const float textHeight = bounds ? bounds->height() : textSize;
        const Size desiredSize{
            textWidth + m_horizontalPadding * 2.0f,
            textHeight + m_verticalPadding * 2.0f
        };
        const Rect rect = resolveRect(frame, desiredSize);
        setComputedSize(rect.width, rect.height);

        m_hovered = isEnabled() && rect.contains(input.mousePosition());
        if (!isEnabled()) {
            m_armed = false;
            m_pressed = false;
        } else {
            if (input.mousePressed(MouseButton::Left)) {
                m_armed = m_hovered;
            }
            m_pressed = m_armed && input.mouseDown(MouseButton::Left);

            if (input.mouseReleased(MouseButton::Left)) {
                const bool activate = m_armed && m_hovered;
                m_armed = false;
                m_pressed = false;
                if (activate && m_onClick) {
                    m_onClick();
                }
            }
        }

        Color background = m_backgroundColor;
        if (!isEnabled()) {
            background = m_disabledColor;
        } else if (m_pressed) {
            background = m_pressedColor;
        } else if (m_hovered) {
            background = m_hoverColor;
        }

        if (m_showBackground) {
            shapeRenderer.drawFilledRect(rect.x, rect.y, rect.width, rect.height, background);
        }
        if (m_showBorder) {
            shapeRenderer.drawRectOutline(rect.x, rect.y, rect.width, rect.height, 1.0f, m_borderColor);
        }

        if (bounds && !m_text.empty()) {
            const float baselineX = rect.x + (rect.width - bounds->width()) * 0.5f - bounds->minX;
            const float baselineY = rect.y + (rect.height - bounds->height()) * 0.5f - bounds->minY;
            Color textColor = m_textColor;
            if (!isEnabled()) {
                textColor.a *= 0.55f;
            }
            textRenderer.renderText(m_text, baselineX, baselineY, textSize, textColor);
        }
    }

    Checkbox::Checkbox(std::string name, std::string text, bool checked)
        : Widget(std::move(name))
        , m_text(std::move(text))
        , m_checked(checked) {
    }

    void Checkbox::render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) {
        const float textSize = m_textSizePixels > 0.0f ? m_textSizePixels : defaultUiTextSize(textRenderer);
        const auto bounds = textRenderer.measureTextBounds(m_text, textSize);
        const float textWidth = bounds ? bounds->width() : 0.0f;
        const float textHeight = bounds ? bounds->height() : textSize;
        const float boxSize = std::max(16.0f, textHeight);
        const float spacing = m_text.empty() ? 0.0f : 8.0f;
        const Rect rect = resolveRect(frame, { boxSize + spacing + textWidth, std::max(boxSize, textHeight) });
        setComputedSize(rect.width, rect.height);

        const bool hovered = isEnabled() && rect.contains(input.mousePosition());
        if (!isEnabled()) {
            m_armed = false;
        } else {
            if (input.mousePressed(MouseButton::Left)) {
                m_armed = hovered;
            }
            if (input.mouseReleased(MouseButton::Left)) {
                const bool activate = m_armed && hovered;
                m_armed = false;
                if (activate) {
                    m_checked = !m_checked;
                    if (m_onChanged) {
                        m_onChanged(m_checked);
                    }
                }
            }
        }

        const float boxY = rect.y + (rect.height - boxSize) * 0.5f;
        const Color boxColor = hovered ? Color{ 0.25f, 0.31f, 0.40f, 1.0f } : Color{ 0.15f, 0.18f, 0.23f, 1.0f };
        shapeRenderer.drawFilledRect(rect.x, boxY, boxSize, boxSize, boxColor);
        shapeRenderer.drawRectOutline(rect.x, boxY, boxSize, boxSize, 1.0f, Color{ 0.55f, 0.65f, 0.78f, 1.0f });

        if (m_checked) {
            const float thickness = std::max(2.0f, boxSize * 0.12f);
            shapeRenderer.drawLine(
                rect.x + boxSize * 0.22f,
                boxY + boxSize * 0.52f,
                rect.x + boxSize * 0.43f,
                boxY + boxSize * 0.28f,
                thickness,
                Color{ 0.45f, 0.85f, 1.0f, 1.0f });
            shapeRenderer.drawLine(
                rect.x + boxSize * 0.43f,
                boxY + boxSize * 0.28f,
                rect.x + boxSize * 0.80f,
                boxY + boxSize * 0.76f,
                thickness,
                Color{ 0.45f, 0.85f, 1.0f, 1.0f });
        }

        if (bounds && !m_text.empty()) {
            const float textX = rect.x + boxSize + spacing - bounds->minX;
            const float textY = rect.y + (rect.height - bounds->height()) * 0.5f - bounds->minY;
            Color textColor = Colors::White;
            if (!isEnabled()) {
                textColor.a = 0.5f;
            }
            textRenderer.renderText(m_text, textX, textY, textSize, textColor);
        }
    }

    Slider::Slider(
        std::string name,
        float minimum,
        float maximum,
        float value)
        : Widget(std::move(name)) {
        setRange(minimum, maximum);
        setValue(value);
    }

    void Slider::setRange(float minimum, float maximum) {
        m_minimum = std::min(minimum, maximum);
        m_maximum = std::max(minimum, maximum);
        if (m_maximum - m_minimum < 0.0001f) {
            m_maximum = m_minimum + 0.0001f;
        }
        m_value = quantize(m_value);
    }

    void Slider::setValue(float value) {
        m_value = quantize(value);
    }

    float Slider::quantize(float value) const {
        float result = std::clamp(value, m_minimum, m_maximum);
        if (m_step > 0.0f) {
            const float safeStep = std::max(m_step, 0.000001f);
            result = m_minimum
                + std::round((result - m_minimum) / safeStep) * safeStep;
            result = std::clamp(result, m_minimum, m_maximum);
        }
        return result;
    }

    void Slider::setValueFromPointer(
        float pointerX,
        const Rect& track) {

        const float ratio = track.width > 0.0f
            ? std::clamp((pointerX - track.x) / track.width, 0.0f, 1.0f)
            : 0.0f;
        const float next = quantize(
            m_minimum + ratio * (m_maximum - m_minimum));
        if (std::abs(next - m_value) <= 0.0001f) {
            return;
        }
        m_value = next;
        if (m_onChanged) {
            m_onChanged(m_value);
        }
    }

    void Slider::render(
        Frame& frame,
        TextRenderer& textRenderer,
        ShapeRenderer& shapeRenderer,
        const Input& input) {

        (void)textRenderer;
        const Rect rect = resolveRect(frame, { 240.0f, 30.0f });
        setComputedSize(rect.width, rect.height);

        const float thumbRadius = std::min(10.0f, rect.height * 0.36f);
        const Rect track{
            rect.x + thumbRadius,
            rect.y + rect.height * 0.5f - 3.0f,
            std::max(0.0f, rect.width - thumbRadius * 2.0f),
            6.0f
        };
        const bool hovered = isEnabled() && rect.contains(input.mousePosition());
        if (!isEnabled()) {
            m_dragging = false;
        } else {
            if (input.mousePressed(MouseButton::Left) && hovered) {
                m_dragging = true;
                setValueFromPointer(input.mousePosition().x, track);
            }
            if (m_dragging && input.mouseDown(MouseButton::Left)) {
                setValueFromPointer(input.mousePosition().x, track);
            }
            if (input.mouseReleased(MouseButton::Left)) {
                m_dragging = false;
            }
        }

        const float ratio = (m_value - m_minimum)
            / (m_maximum - m_minimum);
        const float thumbX = track.x + ratio * track.width;
        Color trackColor = m_trackColor;
        Color fillColor = m_fillColor;
        Color thumbColor = m_thumbColor;
        if (!isEnabled()) {
            trackColor.a *= 0.45f;
            fillColor.a *= 0.45f;
            thumbColor.a *= 0.45f;
        }

        shapeRenderer.drawFilledRect(
            track.x,
            track.y,
            track.width,
            track.height,
            trackColor);
        shapeRenderer.drawFilledRect(
            track.x,
            track.y,
            std::max(0.0f, thumbX - track.x),
            track.height,
            fillColor);
        shapeRenderer.drawFilledCircle(
            thumbX,
            rect.y + rect.height * 0.5f,
            thumbRadius,
            hovered || m_dragging
                ? Color{
                    std::min(thumbColor.r + 0.12f, 1.0f),
                    std::min(thumbColor.g + 0.08f, 1.0f),
                    thumbColor.b,
                    thumbColor.a
                }
                : thumbColor,
            24);
    }

} // namespace WidgeCraft
