#pragma once

#include "WidgeCraft/TextRenderer.hpp"
#include "WidgeCraft/Types.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WidgeCraft {

    class Frame;

    class Widget {
    public:
        explicit Widget(std::string name);
        virtual ~Widget() = default;

        const std::string& getName() const { return m_name; }

        void setVisible(bool visible);
        bool isVisible() const { return m_visible; }

        void setPosition(float x, float y);
        void setPosition(const Position& position) { setPosition(position.x, position.y); }
        Position getPosition() const { return m_position; }

        void setSize(float width, float height);
        void setSize(const Size& size) { setSize(size.x, size.y); }
        Size getSize() const { return m_size; }

        Frame* getParent() const { return m_parent; }

        virtual void render(Frame& frame, TextRenderer& textRenderer) = 0;

        friend class Frame;
        friend class Widgets;

    protected:
        void setParent(Frame* parent);

    private:
        std::string m_name;
        Frame* m_parent = nullptr;
        bool m_visible = true;
        Position m_position{};
        Size m_size{};
    };

    class Widgets {
    public:
        explicit Widgets(Frame& owner);

        template <typename T, typename... Args>
        T& emplace(Args&&... args);

        auto begin() { return m_widgets.begin(); }
        auto end() { return m_widgets.end(); }
        auto begin() const { return m_widgets.begin(); }
        auto end() const { return m_widgets.end(); }
        auto cbegin() const { return m_widgets.cbegin(); }
        auto cend() const { return m_widgets.cend(); }

        std::vector<std::unique_ptr<Widget>>& getAll() { return m_widgets; }
        const std::vector<std::unique_ptr<Widget>>& getAll() const { return m_widgets; }

    private:
        Frame& m_owner;
        std::vector<std::unique_ptr<Widget>> m_widgets;
    };

    class Label : public Widget {
    public:
        Label(std::string name, std::string text = "");

        void setText(std::string text);
        const std::string& getText() const { return m_text; }

        void setColor(TextRenderer::Color color) { m_color = color; }
        TextRenderer::Color getColor() const { return m_color; }

        void setTextSize(float sizePixels) { m_textSizePixels = sizePixels; }
        float getTextSize() const { return m_textSizePixels; }

        void render(Frame& frame, TextRenderer& textRenderer) override;

    private:
        std::string m_text;
        TextRenderer::Color m_color{ 1.0f, 1.0f, 1.0f };
        float m_textSizePixels = 0.0f;
    };

    class Button : public Widget {
    public:
        Button(std::string name, std::string text = "");

        void setText(std::string text);
        const std::string& getText() const { return m_text; }

        void setTextColor(TextRenderer::Color color) { m_textColor = color; }
        TextRenderer::Color getTextColor() const { return m_textColor; }

        void setTextSize(float sizePixels) { m_textSizePixels = sizePixels; }
        float getTextSize() const { return m_textSizePixels; }

        void setBackgroundVisible(bool visible) { m_showBackground = visible; }
        bool isBackgroundVisible() const { return m_showBackground; }

        void setBackgroundColor(Color color) { m_backgroundColor = color; }
        Color getBackgroundColor() const { return m_backgroundColor; }

        void render(Frame& frame, TextRenderer& textRenderer) override;

    private:
        std::string m_text;
        TextRenderer::Color m_textColor{ 1.0f, 1.0f, 1.0f };
        float m_textSizePixels = 0.0f;
        bool m_showBackground = true;
        Color m_backgroundColor{};
    };

    template <typename T, typename... Args>
    T& Widgets::emplace(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        widget->setParent(&m_owner);
        T& reference = *widget;
        m_widgets.emplace_back(std::move(widget));
        return reference;
    }

} // namespace WidgeCraft

