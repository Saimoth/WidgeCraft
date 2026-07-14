#pragma once

#include "WidgeCraft/primitives/Types.hpp"

#include <cstddef>
#include <vector>

namespace WidgeCraft {

    struct ShapeStyle2D {
        Color fillColor{ Colors::White };
        Color edgeColor{ Colors::Transparent };
        float edgeThickness = 0.0f;
        bool fillVisible = true;
        bool edgeVisible = false;
    };

    // ShapeRenderer remains the implementation name for source compatibility.
    // Shapes2D is the preferred public name for new code.
    class ShapeRenderer {
    public:
        ShapeRenderer();
        ~ShapeRenderer();

        ShapeRenderer(const ShapeRenderer&) = delete;
        ShapeRenderer& operator=(const ShapeRenderer&) = delete;
        ShapeRenderer(ShapeRenderer&& other) noexcept;
        ShapeRenderer& operator=(ShapeRenderer&& other) noexcept;

        void setScreenSize(float width, float height);
        void beginFrame();
        void flush();

        // Applies a uniform logical-scene transform while geometry is queued.
        // Vertex positions, dimensions and edge thicknesses all receive the
        // same scale, preserving shape aspect ratios.
        void setTransform(const Vec2& offset, float uniformScale);
        void resetTransform();
        Vec2 getTransformOffset() const { return m_transformOffset; }
        float getTransformScale() const { return m_transformScale; }

        void reserve(std::size_t triangleCount);

        void drawPoint(float x, float y, float size, const Color& color);
        void drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& color);
        void drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Color& color);
        void drawFilledRect(float x, float y, float width, float height, const Color& color);
        void drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color);
        void drawFilledCircle(float centerX, float centerY, float radius, const Color& color, int segments = 0);
        void drawCircleOutline(float centerX, float centerY, float radius, float thickness, const Color& color, int segments = 0);

        void drawTriangleOutline(
            const Vec2& a,
            const Vec2& b,
            const Vec2& c,
            float thickness,
            const Color& color) {
            drawLine(a.x, a.y, b.x, b.y, thickness, color);
            drawLine(b.x, b.y, c.x, c.y, thickness, color);
            drawLine(c.x, c.y, a.x, a.y, thickness, color);
        }

        void drawTriangle(
            const Vec2& a,
            const Vec2& b,
            const Vec2& c,
            const ShapeStyle2D& style) {
            if (style.fillVisible) {
                drawTriangle(a, b, c, style.fillColor);
            }
            if (style.edgeVisible && style.edgeThickness > 0.0f) {
                drawTriangleOutline(a, b, c, style.edgeThickness, style.edgeColor);
            }
        }

        void drawRect(const Rect& rect, const ShapeStyle2D& style) {
            if (style.fillVisible) {
                drawFilledRect(rect.x, rect.y, rect.width, rect.height, style.fillColor);
            }
            if (style.edgeVisible && style.edgeThickness > 0.0f) {
                drawRectOutline(
                    rect.x,
                    rect.y,
                    rect.width,
                    rect.height,
                    style.edgeThickness,
                    style.edgeColor);
            }
        }

        void drawCircle(
            const Vec2& center,
            float radius,
            const ShapeStyle2D& style,
            int segments = 0) {
            if (style.fillVisible) {
                drawFilledCircle(center.x, center.y, radius, style.fillColor, segments);
            }
            if (style.edgeVisible && style.edgeThickness > 0.0f) {
                drawCircleOutline(
                    center.x,
                    center.y,
                    radius,
                    style.edgeThickness,
                    style.edgeColor,
                    segments);
            }
        }

        std::size_t getQueuedTriangleCount() const { return m_vertices.size() / 3; }

    private:
        struct Vertex {
            float x = 0.0f;
            float y = 0.0f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;
        };

        void addVertex(const Vec2& position, const Color& color);
        void moveFrom(ShapeRenderer&& other) noexcept;
        void cleanup();

        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_shader = 0;
        int m_uniformScreenSize = -1;
        float m_screenWidth = 0.0f;
        float m_screenHeight = 0.0f;
        Vec2 m_transformOffset{};
        float m_transformScale = 1.0f;
        std::vector<Vertex> m_vertices;
    };

    using Shapes2D = ShapeRenderer;

} // namespace WidgeCraft
