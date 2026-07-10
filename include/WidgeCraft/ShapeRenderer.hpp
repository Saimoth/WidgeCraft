#pragma once

#include "WidgeCraft/Types.hpp"

#include <cstddef>
#include <vector>

namespace WidgeCraft {

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

        void reserve(std::size_t triangleCount);

        void drawPoint(float x, float y, float size, const Color& color);
        void drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& color);
        void drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Color& color);
        void drawFilledRect(float x, float y, float width, float height, const Color& color);
        void drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color);
        void drawFilledCircle(float centerX, float centerY, float radius, const Color& color, int segments = 0);
        void drawCircleOutline(float centerX, float centerY, float radius, float thickness, const Color& color, int segments = 0);

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
        std::vector<Vertex> m_vertices;
    };

} // namespace WidgeCraft
