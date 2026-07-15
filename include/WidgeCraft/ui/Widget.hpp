#pragma once

#include "WidgeCraft/Input.hpp"
#include "WidgeCraft/TextRenderer.hpp"
#include "WidgeCraft/Types.hpp"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace WidgeCraft {

    class Frame;
    class ShapeRenderer;

    enum class HorizontalAlignment {
        Left,
        Center,
        Right
    };

    class Widget {
    public:
        explicit Widget(std::string name);
        virtual ~Widget() = default;

        Widget(const Widget&) = delete;
        Widget& operator=(const Widget&) = delete;

        const std::string& getName() const { return m_name; }

        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }

        void setPosition(float x, float y);
        void setPosition(const Position& position) { setPosition(position.x, position.y); }
        Position getPosition() const { return m_position; }

        void setSize(float width, float height);
        void setSize(const Size& size) { setSize(size.x, size.y); }
        void clearExplicitSize() { m_hasExplicitSize = false; }
        Size getSize() const { return m_size; }
        bool hasExplicitSize() const { return m_hasExplicitSize; }

        void setAnchor(Anchor anchor) { m_anchor = anchor; }
        Anchor getAnchor() const { return m_anchor; }

        Frame* getParent() const { return m_parent; }
        Rect getAbsoluteRect() const;

        virtual void render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) = 0;

        friend class Frame;
        friend class Widgets;

    protected:
        void setParent(Frame* parent);
        void setComputedSize(float width, float height);
        Rect resolveRect(const Frame& frame, const Size& desiredSize) const;

    private:
        std::string m_name;
        Frame* m_parent = nullptr;
        bool m_visible = true;
        bool m_enabled = true;
        Position m_position{};
        Size m_size{};
        bool m_hasExplicitSize = false;
        Anchor m_anchor = Anchor::BottomLeft;
    };

    class Widgets {
    public:
        explicit Widgets(Frame& owner);

        template <typename T, typename... Args>
        T& emplace(Args&&... args);

        Widget* find(const std::string& name);
        const Widget* find(const std::string& name) const;
        bool remove(const std::string& name);

        auto begin() { return m_widgets.begin(); }
        auto end() { return m_widgets.end(); }
        auto begin() const { return m_widgets.begin(); }
        auto end() const { return m_widgets.end(); }

        std::vector<std::unique_ptr<Widget>>& getAll() { return m_widgets; }
        const std::vector<std::unique_ptr<Widget>>& getAll() const { return m_widgets; }

    private:
        Frame& m_owner;
        std::vector<std::unique_ptr<Widget>> m_widgets;
    };

    class Label : public Widget {
    public:
        Label(std::string name, std::string text = "");

        void setText(std::string text) { m_text = std::move(text); }
        const std::string& getText() const { return m_text; }

        void setColor(Color color) { m_color = color; }
        Color getColor() const { return m_color; }

        void setTextSize(float sizePixels) { m_textSizePixels = sizePixels; }
        float getTextSize() const { return m_textSizePixels; }

        void setAlignment(HorizontalAlignment alignment) { m_alignment = alignment; }
        HorizontalAlignment getAlignment() const { return m_alignment; }

        void render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) override;

    private:
        std::string m_text;
        Color m_color{ Colors::White };
        float m_textSizePixels = 0.0f;
        HorizontalAlignment m_alignment = HorizontalAlignment::Left;
    };

    class Button : public Widget {
    public:
        using ClickCallback = std::function<void()>;

        Button(std::string name, std::string text = "");

        void setText(std::string text) { m_text = std::move(text); }
        const std::string& getText() const { return m_text; }

        void setTextColor(Color color) { m_textColor = color; }
        Color getTextColor() const { return m_textColor; }
        void setTextSize(float sizePixels) { m_textSizePixels = sizePixels; }

        void setBackgroundVisible(bool visible) { m_showBackground = visible; }
        void setBackgroundColor(Color color) { m_backgroundColor = color; }
        void setHoverColor(Color color) { m_hoverColor = color; }
        void setPressedColor(Color color) { m_pressedColor = color; }
        void setDisabledColor(Color color) { m_disabledColor = color; }
        void setBorderColor(Color color) { m_borderColor = color; }
        void setBorderVisible(bool visible) { m_showBorder = visible; }
        void setPadding(float horizontal, float vertical);

        void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }
        bool isHovered() const { return m_hovered; }
        bool isPressed() const { return m_pressed; }

        void render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) override;

    private:
        std::string m_text;
        Color m_textColor{ Colors::White };
        Color m_backgroundColor{ 0.20f, 0.24f, 0.30f, 1.0f };
        Color m_hoverColor{ 0.27f, 0.34f, 0.43f, 1.0f };
        Color m_pressedColor{ 0.13f, 0.17f, 0.23f, 1.0f };
        Color m_disabledColor{ 0.20f, 0.20f, 0.20f, 0.65f };
        Color m_borderColor{ 0.55f, 0.65f, 0.78f, 1.0f };
        float m_textSizePixels = 0.0f;
        float m_horizontalPadding = 12.0f;
        float m_verticalPadding = 7.0f;
        bool m_showBackground = true;
        bool m_showBorder = true;
        bool m_hovered = false;
        bool m_pressed = false;
        bool m_armed = false;
        ClickCallback m_onClick;
    };

    class Checkbox : public Widget {
    public:
        using ChangeCallback = std::function<void(bool)>;

        Checkbox(std::string name, std::string text = "", bool checked = false);

        void setText(std::string text) { m_text = std::move(text); }
        const std::string& getText() const { return m_text; }
        void setChecked(bool checked) { m_checked = checked; }
        bool isChecked() const { return m_checked; }
        void setTextSize(float sizePixels) { m_textSizePixels = sizePixels; }
        void setOnChanged(ChangeCallback callback) { m_onChanged = std::move(callback); }

        void render(Frame& frame, TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input) override;

    private:
        std::string m_text;
        bool m_checked = false;
        bool m_armed = false;
        float m_textSizePixels = 0.0f;
        ChangeCallback m_onChanged;
    };

    class Slider : public Widget {
    public:
        using ChangeCallback = std::function<void(float)>;

        Slider(
            std::string name,
            float minimum = 0.0f,
            float maximum = 1.0f,
            float value = 0.0f);

        void setRange(float minimum, float maximum);
        float getMinimum() const { return m_minimum; }
        float getMaximum() const { return m_maximum; }

        void setValue(float value);
        float getValue() const { return m_value; }

        void setStep(float step) { m_step = std::max(step, 0.0f); }
        float getStep() const { return m_step; }

        void setTrackColor(Color color) { m_trackColor = color; }
        void setFillColor(Color color) { m_fillColor = color; }
        void setThumbColor(Color color) { m_thumbColor = color; }
        void setOnChanged(ChangeCallback callback) {
            m_onChanged = std::move(callback);
        }

        bool isDragging() const { return m_dragging; }

        void render(
            Frame& frame,
            TextRenderer& textRenderer,
            ShapeRenderer& shapeRenderer,
            const Input& input) override;

    private:
        float quantize(float value) const;
        void setValueFromPointer(float pointerX, const Rect& track);

        float m_minimum = 0.0f;
        float m_maximum = 1.0f;
        float m_value = 0.0f;
        float m_step = 0.0f;
        bool m_dragging = false;
        Color m_trackColor{ 0.13f, 0.18f, 0.24f, 1.0f };
        Color m_fillColor{ 0.18f, 0.58f, 0.82f, 1.0f };
        Color m_thumbColor{ 0.78f, 0.92f, 1.0f, 1.0f };
        ChangeCallback m_onChanged;
    };

    template <typename T, typename... Args>
    T& Widgets::emplace(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        if (find(widget->getName())) {
            throw std::invalid_argument("A widget named '" + widget->getName() + "' already exists in this frame");
        }
        widget->setParent(&m_owner);
        T& reference = *widget;
        m_widgets.emplace_back(std::move(widget));
        return reference;
    }

} // namespace WidgeCraft
