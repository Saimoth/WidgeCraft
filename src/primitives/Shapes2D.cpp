#include "WidgeCraft/primitives/Shapes2D.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    constexpr float kPi = 3.14159265358979323846f;

    GLuint compileShader(GLenum type, const char* source) {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint logLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error("Failed to compile shape shader: " + log);
        }
        return shader;
    }

    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
        const GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            glDeleteProgram(program);
            throw std::runtime_error("Failed to link shape shader: " + log);
        }
        return program;
    }

    int chooseCircleSegments(float radius, int requestedSegments) {
        if (requestedSegments > 2) {
            return requestedSegments;
        }
        return std::clamp(static_cast<int>(std::ceil(radius * 0.75f)), 12, 128);
    }

} // namespace

namespace WidgeCraft {

    ShapeRenderer::ShapeRenderer() {
        static constexpr char kVertexShader[] = R"(
            #version 330 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec4 aColor;

            uniform vec2 uScreenSize;
            out vec4 vColor;

            void main() {
                vec2 ndc = vec2(
                    (aPosition.x / uScreenSize.x) * 2.0 - 1.0,
                    (aPosition.y / uScreenSize.y) * 2.0 - 1.0
                );
                gl_Position = vec4(ndc, 0.0, 1.0);
                vColor = aColor;
            }
        )";

        static constexpr char kFragmentShader[] = R"(
            #version 330 core
            in vec4 vColor;
            out vec4 fragColor;

            void main() {
                fragColor = vColor;
            }
        )";

        const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShader);
        const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
        m_shader = linkProgram(vertexShader, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_uniformScreenSize = glGetUniformLocation(m_shader, "uScreenSize");
        m_vertices.reserve(4096);
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

    void ShapeRenderer::setScreenSize(float width, float height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    void ShapeRenderer::setTransform(
        const Vec2& offset,
        float uniformScale) {

        m_transformOffset = offset;
        m_transformScale = std::max(uniformScale, 0.0f);
    }

    void ShapeRenderer::resetTransform() {
        m_transformOffset = {};
        m_transformScale = 1.0f;
    }

    void ShapeRenderer::beginFrame() {
        m_vertices.clear();
        resetTransform();
    }

    void ShapeRenderer::flush() {
        if (m_vertices.empty() || m_screenWidth <= 0.0f || m_screenHeight <= 0.0f) {
            m_vertices.clear();
            return;
        }

        glUseProgram(m_shader);
        glUniform2f(m_uniformScreenSize, m_screenWidth, m_screenHeight);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
            m_vertices.data(),
            GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        m_vertices.clear();
    }

    void ShapeRenderer::reserve(std::size_t triangleCount) {
        m_vertices.reserve(triangleCount * 3);
    }

    void ShapeRenderer::drawPoint(float x, float y, float size, const Color& color) {
        if (size <= 0.0f) {
            return;
        }
        drawFilledCircle(x, y, size * 0.5f, color);
    }

    void ShapeRenderer::drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& color) {
        if (thickness <= 0.0f) {
            return;
        }

        const Vec2 start{ x1, y1 };
        const Vec2 end{ x2, y2 };
        const Vec2 delta = end - start;
        const float segmentLength = length(delta);
        if (segmentLength <= 1.0e-6f) {
            drawPoint(x1, y1, thickness, color);
            return;
        }

        const Vec2 normal{ -delta.y / segmentLength, delta.x / segmentLength };
        const Vec2 offset = normal * (thickness * 0.5f);
        const Vec2 a = start - offset;
        const Vec2 b = start + offset;
        const Vec2 c = end + offset;
        const Vec2 d = end - offset;

        drawTriangle(a, b, c, color);
        drawTriangle(a, c, d, color);
    }

    void ShapeRenderer::drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Color& color) {
        addVertex(a, color);
        addVertex(b, color);
        addVertex(c, color);
    }

    void ShapeRenderer::drawMesh(
        const Mesh2D& mesh,
        const Vec2& position,
        float rotationRadians,
        const Vec2& scale,
        const ShapeStyle2D& style) {

        if (!mesh.isValid()) {
            return;
        }

        const float cosine = std::cos(rotationRadians);
        const float sine = std::sin(rotationRadians);
        const auto transformPoint = [&](const Vec2& point) {
            const Vec2 scaled{ point.x * scale.x, point.y * scale.y };
            return Vec2{
                scaled.x * cosine - scaled.y * sine + position.x,
                scaled.x * sine + scaled.y * cosine + position.y
            };
        };

        for (std::size_t index = 0;
             index + 2U < mesh.indices.size();
             index += 3U) {
            drawTriangle(
                transformPoint(mesh.vertices[mesh.indices[index]]),
                transformPoint(mesh.vertices[mesh.indices[index + 1U]]),
                transformPoint(mesh.vertices[mesh.indices[index + 2U]]),
                style);
        }
    }

    void ShapeRenderer::drawFilledRect(float x, float y, float width, float height, const Color& color) {
        if (width <= 0.0f || height <= 0.0f) {
            return;
        }

        const Vec2 bottomLeft{ x, y };
        const Vec2 bottomRight{ x + width, y };
        const Vec2 topRight{ x + width, y + height };
        const Vec2 topLeft{ x, y + height };
        drawTriangle(bottomLeft, bottomRight, topRight, color);
        drawTriangle(bottomLeft, topRight, topLeft, color);
    }

    void ShapeRenderer::drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color) {
        if (width <= 0.0f || height <= 0.0f || thickness <= 0.0f) {
            return;
        }

        const float clamped = std::min(thickness, std::min(width, height) * 0.5f);
        drawFilledRect(x, y, width, clamped, color);
        drawFilledRect(x, y + height - clamped, width, clamped, color);
        drawFilledRect(x, y + clamped, clamped, height - clamped * 2.0f, color);
        drawFilledRect(x + width - clamped, y + clamped, clamped, height - clamped * 2.0f, color);
    }

    void ShapeRenderer::drawFilledCircle(float centerX, float centerY, float radius, const Color& color, int segments) {
        if (radius <= 0.0f) {
            return;
        }

        const int segmentCount = chooseCircleSegments(radius, segments);
        const Vec2 center{ centerX, centerY };
        for (int index = 0; index < segmentCount; ++index) {
            const float angle0 = (static_cast<float>(index) / static_cast<float>(segmentCount)) * (2.0f * kPi);
            const float angle1 = (static_cast<float>(index + 1) / static_cast<float>(segmentCount)) * (2.0f * kPi);
            const Vec2 a{ centerX + std::cos(angle0) * radius, centerY + std::sin(angle0) * radius };
            const Vec2 b{ centerX + std::cos(angle1) * radius, centerY + std::sin(angle1) * radius };
            drawTriangle(center, a, b, color);
        }
    }

    void ShapeRenderer::drawCircleOutline(float centerX, float centerY, float radius, float thickness, const Color& color, int segments) {
        if (radius <= 0.0f || thickness <= 0.0f) {
            return;
        }

        const int segmentCount = chooseCircleSegments(radius, segments);
        const float innerRadius = std::max(0.0f, radius - thickness);
        for (int index = 0; index < segmentCount; ++index) {
            const float angle0 = (static_cast<float>(index) / static_cast<float>(segmentCount)) * (2.0f * kPi);
            const float angle1 = (static_cast<float>(index + 1) / static_cast<float>(segmentCount)) * (2.0f * kPi);

            const Vec2 outer0{ centerX + std::cos(angle0) * radius, centerY + std::sin(angle0) * radius };
            const Vec2 outer1{ centerX + std::cos(angle1) * radius, centerY + std::sin(angle1) * radius };
            const Vec2 inner0{ centerX + std::cos(angle0) * innerRadius, centerY + std::sin(angle0) * innerRadius };
            const Vec2 inner1{ centerX + std::cos(angle1) * innerRadius, centerY + std::sin(angle1) * innerRadius };

            drawTriangle(inner0, outer0, outer1, color);
            drawTriangle(inner0, outer1, inner1, color);
        }
    }

    void ShapeRenderer::addVertex(const Vec2& position, const Color& color) {
        const Vec2 transformed =
            m_transformOffset + position * m_transformScale;
        m_vertices.push_back({
            transformed.x,
            transformed.y,
            color.r,
            color.g,
            color.b,
            color.a
        });
    }

    void ShapeRenderer::moveFrom(ShapeRenderer&& other) noexcept {
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_shader = other.m_shader;
        m_uniformScreenSize = other.m_uniformScreenSize;
        m_screenWidth = other.m_screenWidth;
        m_screenHeight = other.m_screenHeight;
        m_transformOffset = other.m_transformOffset;
        m_transformScale = other.m_transformScale;
        m_vertices = std::move(other.m_vertices);

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_shader = 0;
        other.m_uniformScreenSize = -1;
        other.m_screenWidth = 0.0f;
        other.m_screenHeight = 0.0f;
        other.resetTransform();
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
        m_vertices.clear();
    }

} // namespace WidgeCraft
