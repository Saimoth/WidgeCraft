#include "WidgeCraft/ShapeRenderer.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace {

    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint logLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<size_t>(logLength > 0 ? logLength : 1), '\0');
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error("Failed to compile rectangle shader: " + log);
        }

        return shader;
    }

    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint status = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<size_t>(logLength > 0 ? logLength : 1), '\0');
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            glDeleteProgram(program);
            throw std::runtime_error("Failed to link rectangle shader: " + log);
        }

        return program;
    }

} // namespace

namespace WidgeCraft {

    ShapeRenderer::ShapeRenderer() {
        static constexpr char kVertexShaderSrc[] = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;

            uniform vec2 uScreenSize;

            void main() {
                vec2 normalized = vec2(
                    (aPos.x / uScreenSize.x) * 2.0 - 1.0,
                    (aPos.y / uScreenSize.y) * 2.0 - 1.0
                );
                gl_Position = vec4(normalized, 0.0, 1.0);
            }
        )";

        static constexpr char kFragmentShaderSrc[] = R"(
            #version 330 core
            uniform vec4 uColor;
            out vec4 FragColor;

            void main() {
                FragColor = uColor;
            }
        )";

        const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSrc);
        const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
        m_shader = linkProgram(vertexShader, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, reinterpret_cast<void*>(0));
        glBindVertexArray(0);

        m_uniformScreenSize = glGetUniformLocation(m_shader, "uScreenSize");
        m_uniformColor = glGetUniformLocation(m_shader, "uColor");
    }

    ShapeRenderer::~ShapeRenderer() {
        cleanup();
    }

    ShapeRenderer::ShapeRenderer(ShapeRenderer&& other) noexcept {
        moveFrom(std::move(other));
    }

    ShapeRenderer& ShapeRenderer::operator=(ShapeRenderer&& other) noexcept {
        if (this != &other) {
            cleanup();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void ShapeRenderer::moveFrom(ShapeRenderer&& other) noexcept {
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_shader = other.m_shader;
        m_uniformScreenSize = other.m_uniformScreenSize;
        m_uniformColor = other.m_uniformColor;
        m_screenWidth = other.m_screenWidth;
        m_screenHeight = other.m_screenHeight;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_shader = 0;
        other.m_uniformScreenSize = -1;
        other.m_uniformColor = -1;
        other.m_screenWidth = 0.0f;
        other.m_screenHeight = 0.0f;
    }

    void ShapeRenderer::cleanup() {
        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
        if (m_shader != 0) {
            glDeleteProgram(m_shader);
            m_shader = 0;
        }
        m_uniformScreenSize = -1;
        m_uniformColor = -1;
        m_screenWidth = 0.0f;
        m_screenHeight = 0.0f;
    }

    void ShapeRenderer::setScreenSize(float width, float height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    void ShapeRenderer::drawFilledRect(float x, float y, float width, float height, const Color& color) const {
        if (width <= 0.0f || height <= 0.0f || m_screenWidth <= 0.0f || m_screenHeight <= 0.0f) {
            return;
        }

        const std::array<float, 8> vertices{
            x, y,
            x + width, y,
            x, y + height,
            x + width, y + height,
        };

        glUseProgram(m_shader);
        glUniform2f(m_uniformScreenSize, m_screenWidth, m_screenHeight);
        glUniform4f(m_uniformColor, color.r, color.g, color.b, color.a);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void ShapeRenderer::drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color) const {
        if (width <= 0.0f || height <= 0.0f || thickness <= 0.0f) {
            return;
        }

        const float clampedThickness = std::min(thickness, std::min(width, height));

        drawFilledRect(x, y, width, clampedThickness, color);
        drawFilledRect(x, y + height - clampedThickness, width, clampedThickness, color);
        drawFilledRect(x, y, clampedThickness, height, color);
        drawFilledRect(x + width - clampedThickness, y, clampedThickness, height, color);
    }

} // namespace WidgeCraft

