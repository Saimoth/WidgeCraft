#pragma once

#include "WidgeCraft/Types.hpp"

#include <glad/glad.h>

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

        void drawFilledRect(float x, float y, float width, float height, const Color& color) const;
        void drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color) const;

    private:
        void moveFrom(ShapeRenderer&& other) noexcept;
        void cleanup();

        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_shader = 0;
        GLint m_uniformScreenSize = -1;
        GLint m_uniformColor = -1;
        float m_screenWidth = 0.0f;
        float m_screenHeight = 0.0f;
    };

} // namespace WidgeCraft

