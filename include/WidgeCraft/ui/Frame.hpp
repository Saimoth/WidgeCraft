#pragma once

#include "WidgeCraft/Input.hpp"
#include "WidgeCraft/TextRenderer.hpp"
#include "WidgeCraft/Types.hpp"
#include "WidgeCraft/Widget.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace WidgeCraft {

    class ShapeRenderer;

    class Frame {
    public:
        class TextBatch {
        public:
            struct Command {
                std::string text;
                Position position;
                float sizePixels = 0.0f;
                Color color{ Colors::White };
            };

            void addText(std::string text, float x, float y, float sizePixels, Color color = Colors::White);
            void clear();
            const std::vector<Command>& getCommands() const { return m_commands; }

        private:
            std::vector<Command> m_commands;
        };

        Frame(std::string name, Frame* parent = nullptr);
        ~Frame() = default;

        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;
        Frame(Frame&&) noexcept = default;
        Frame& operator=(Frame&&) noexcept = default;

        const std::string& getName() const { return m_name; }

        void setVisible(bool visible);
        bool isVisible() const { return m_visible; }

        void setBackgroundVisible(bool visible) { m_showBackground = visible; }
        bool isBackgroundVisible() const { return m_showBackground; }
        void setBorderVisible(bool visible) { m_showBorder = visible; }
        bool isBorderVisible() const { return m_showBorder; }
        void setClipContents(bool clipContents) { m_clipContents = clipContents; }
        bool clipsContents() const { return m_clipContents; }

        void setBackgroundColor(Color color) { m_backgroundColor = color; }
        Color getBackgroundColor() const { return m_backgroundColor; }
        void setBorderColor(Color color) { m_borderColor = color; }
        Color getBorderColor() const { return m_borderColor; }
        void setBorderThickness(float thickness) { m_borderThickness = thickness; }
        float getBorderThickness() const { return m_borderThickness; }

        void setPosition(float x, float y);
        void setPosition(const Position& position) { setPosition(position.x, position.y); }
        Position getPosition() const { return m_position; }

        void setSize(float width, float height);
        void setSize(const Size& size) { setSize(size.x, size.y); }
        Size getSize() const { return m_size; }

        void setAnchor(Anchor anchor) { m_anchor = anchor; }
        Anchor getAnchor() const { return m_anchor; }

        Position getAbsolutePosition() const;
        Rect getAbsoluteRect() const;

        Frame& createChildFrame(const std::string& name);
        Frame* findChildFrame(const std::string& name);
        const Frame* findChildFrame(const std::string& name) const;
        bool removeChildFrame(const std::string& name);

        template <typename T, typename... Args>
        T& addWidget(Args&&... args);

        Widget* findWidget(const std::string& name) { return m_widgets.find(name); }
        const Widget* findWidget(const std::string& name) const { return m_widgets.find(name); }
        bool removeWidget(const std::string& name) { return m_widgets.remove(name); }

        TextBatch& getTextBatch() { return m_textBatch; }
        const TextBatch& getTextBatch() const { return m_textBatch; }

        const std::vector<std::unique_ptr<Frame>>& getChildren() const { return m_children; }
        Widgets& getWidgets() { return m_widgets; }
        const Widgets& getWidgets() const { return m_widgets; }

        Frame* getParent() const { return m_parent; }
        void setDeletable(bool deletable) { m_deletable = deletable; }
        bool canBeDeleted() const { return m_deletable; }

        void render(TextRenderer& textRenderer, ShapeRenderer& shapeRenderer, const Input& input);

    private:
        void renderInternal(
            TextRenderer& textRenderer,
            ShapeRenderer& shapeRenderer,
            const Input& input,
            const Rect& parentClip,
            bool isRoot);
        void clearTextBatchesRecursive();

        std::string m_name;
        Frame* m_parent = nullptr;
        bool m_visible = true;
        bool m_showBackground = true;
        bool m_showBorder = false;
        bool m_clipContents = true;
        bool m_deletable = true;
        Color m_backgroundColor{ Colors::Grey };
        Color m_borderColor{ Colors::Black };
        float m_borderThickness = 1.0f;
        Position m_position{ 0.0f, 0.0f };
        Size m_size{};
        Anchor m_anchor = Anchor::BottomLeft;
        TextBatch m_textBatch;
        Widgets m_widgets;
        std::vector<std::unique_ptr<Frame>> m_children;
    };

    template <typename T, typename... Args>
    T& Frame::addWidget(Args&&... args) {
        return m_widgets.template emplace<T>(std::forward<Args>(args)...);
    }

} // namespace WidgeCraft
