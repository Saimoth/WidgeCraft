#include "WidgeCraft/primitives/Shapes3D.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace WidgeCraft {

    namespace {
        constexpr std::string_view kVertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPosition;
            layout (location = 1) in vec4 aColor;

            uniform mat4 uViewProjection;
            out vec4 vColor;

            void main() {
                gl_Position = uViewProjection * vec4(aPosition, 1.0);
                vColor = aColor;
            }
        )";

        constexpr std::string_view kFragmentShader = R"(
            #version 330 core
            in vec4 vColor;
            out vec4 fragColor;

            void main() {
                fragColor = vColor;
            }
        )";
    } // namespace

    Shapes3D::Shapes3D()
        : m_shader(kVertexShader, kFragmentShader) {

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, x)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, r)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_uniformViewProjection = m_shader.uniformLocation("uViewProjection");
        reserve(256, 256);
    }

    Shapes3D::~Shapes3D() {
        cleanup();
    }

    Shapes3D::Shapes3D(Shapes3D&& other) noexcept {
        moveFrom(std::move(other));
    }

    Shapes3D& Shapes3D::operator=(Shapes3D&& other) noexcept {
        if (this != &other) {
            cleanup();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void Shapes3D::beginFrame() {
        m_triangles.clear();
        m_lines.clear();
        m_lineVertices.clear();
    }

    void Shapes3D::flush() {
        if (m_triangles.empty() && m_lines.empty()) {
            return;
        }

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);

        m_shader.use();
        glUniformMatrix4fv(
            m_uniformViewProjection,
            1,
            GL_FALSE,
            m_viewProjection.values.data());
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        if (!m_triangles.empty()) {
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_triangles.size() * sizeof(Vertex)),
                m_triangles.data(),
                GL_DYNAMIC_DRAW);
            glDrawArrays(
                GL_TRIANGLES,
                0,
                static_cast<GLsizei>(m_triangles.size()));
        }

        if (!m_lines.empty()) {
            m_lineVertices.clear();
            m_lineVertices.reserve(m_lines.size() * 2U);
            for (const LineCommand& line : m_lines) {
                m_lineVertices.push_back(line.start);
                m_lineVertices.push_back(line.end);
            }

            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_lineVertices.size() * sizeof(Vertex)),
                m_lineVertices.data(),
                GL_DYNAMIC_DRAW);

            for (std::size_t index = 0; index < m_lines.size(); ++index) {
                glLineWidth(std::max(1.0f, m_lines[index].thickness));
                glDrawArrays(
                    GL_LINES,
                    static_cast<GLint>(index * 2U),
                    2);
            }
            glLineWidth(1.0f);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        ShaderProgram::stopUsing();
        glDisable(GL_DEPTH_TEST);

        m_triangles.clear();
        m_lines.clear();
        m_lineVertices.clear();
    }

    void Shapes3D::reserve(
        std::size_t triangleCount,
        std::size_t lineCount) {
        m_triangles.reserve(triangleCount * 3U);
        m_lines.reserve(lineCount);
        m_lineVertices.reserve(lineCount * 2U);
    }

    void Shapes3D::drawLine(
        const Vec3& start,
        const Vec3& end,
        float thickness,
        const Color& color) {

        if (thickness <= 0.0f || lengthSquared(end - start) <= 1.0e-12f) {
            return;
        }
        m_lines.push_back({
            makeVertex(start, color),
            makeVertex(end, color),
            thickness
        });
    }

    void Shapes3D::drawTriangle(
        const Vec3& a,
        const Vec3& b,
        const Vec3& c,
        const Color& color) {
        appendTriangle(a, b, c, color);
    }

    void Shapes3D::drawTriangle(
        const Vec3& a,
        const Vec3& b,
        const Vec3& c,
        const ShapeStyle3D& style) {

        if (style.fillVisible) {
            appendTriangle(a, b, c, style.fillColor);
        }
        if (style.edgeVisible && style.edgeThickness > 0.0f) {
            drawLine(a, b, style.edgeThickness, style.edgeColor);
            drawLine(b, c, style.edgeThickness, style.edgeColor);
            drawLine(c, a, style.edgeThickness, style.edgeColor);
        }
    }

    void Shapes3D::drawQuad(
        const Vec3& a,
        const Vec3& b,
        const Vec3& c,
        const Vec3& d,
        const ShapeStyle3D& style) {

        if (style.fillVisible) {
            appendTriangle(a, b, c, style.fillColor);
            appendTriangle(a, c, d, style.fillColor);
        }
        if (style.edgeVisible && style.edgeThickness > 0.0f) {
            drawLine(a, b, style.edgeThickness, style.edgeColor);
            drawLine(b, c, style.edgeThickness, style.edgeColor);
            drawLine(c, d, style.edgeThickness, style.edgeColor);
            drawLine(d, a, style.edgeThickness, style.edgeColor);
        }
    }

    void Shapes3D::drawBox(
        const Vec3& minimum,
        const Vec3& maximum,
        const ShapeStyle3D& style) {

        const std::array<Vec3, 8> corners{
            Vec3{ minimum.x, minimum.y, minimum.z },
            Vec3{ maximum.x, minimum.y, minimum.z },
            Vec3{ maximum.x, maximum.y, minimum.z },
            Vec3{ minimum.x, maximum.y, minimum.z },
            Vec3{ minimum.x, minimum.y, maximum.z },
            Vec3{ maximum.x, minimum.y, maximum.z },
            Vec3{ maximum.x, maximum.y, maximum.z },
            Vec3{ minimum.x, maximum.y, maximum.z }
        };

        if (style.fillVisible) {
            appendTriangle(corners[0], corners[3], corners[2], style.fillColor);
            appendTriangle(corners[0], corners[2], corners[1], style.fillColor);
            appendTriangle(corners[4], corners[5], corners[6], style.fillColor);
            appendTriangle(corners[4], corners[6], corners[7], style.fillColor);
            appendTriangle(corners[0], corners[4], corners[7], style.fillColor);
            appendTriangle(corners[0], corners[7], corners[3], style.fillColor);
            appendTriangle(corners[1], corners[2], corners[6], style.fillColor);
            appendTriangle(corners[1], corners[6], corners[5], style.fillColor);
            appendTriangle(corners[0], corners[1], corners[5], style.fillColor);
            appendTriangle(corners[0], corners[5], corners[4], style.fillColor);
            appendTriangle(corners[3], corners[7], corners[6], style.fillColor);
            appendTriangle(corners[3], corners[6], corners[2], style.fillColor);
        }

        if (style.edgeVisible && style.edgeThickness > 0.0f) {
            appendBoxEdges(
                corners.data(),
                style.edgeThickness,
                style.edgeColor);
        }
    }

    void Shapes3D::drawCube(
        const Vec3& center,
        float size,
        const ShapeStyle3D& style) {

        if (size <= 0.0f) {
            return;
        }
        const Vec3 half{ size * 0.5f, size * 0.5f, size * 0.5f };
        drawBox(center - half, center + half, style);
    }

    Shapes3D::Vertex Shapes3D::makeVertex(
        const Vec3& position,
        const Color& color) {
        return {
            position.x,
            position.y,
            position.z,
            color.r,
            color.g,
            color.b,
            color.a
        };
    }

    void Shapes3D::appendTriangle(
        const Vec3& a,
        const Vec3& b,
        const Vec3& c,
        const Color& color) {
        m_triangles.push_back(makeVertex(a, color));
        m_triangles.push_back(makeVertex(b, color));
        m_triangles.push_back(makeVertex(c, color));
    }

    void Shapes3D::appendBoxEdges(
        const Vec3* corners,
        float thickness,
        const Color& color) {

        static constexpr int edges[12][2] = {
            { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
            { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
            { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
        };

        for (const auto& edge : edges) {
            drawLine(
                corners[edge[0]],
                corners[edge[1]],
                thickness,
                color);
        }
    }

    void Shapes3D::cleanup() {
        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
        m_shader.reset();
        m_uniformViewProjection = -1;
        m_triangles.clear();
        m_lines.clear();
        m_lineVertices.clear();
    }

    void Shapes3D::moveFrom(Shapes3D&& other) noexcept {
        m_shader = std::move(other.m_shader);
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_uniformViewProjection = other.m_uniformViewProjection;
        m_viewProjection = other.m_viewProjection;
        m_triangles = std::move(other.m_triangles);
        m_lines = std::move(other.m_lines);
        m_lineVertices = std::move(other.m_lineVertices);

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_uniformViewProjection = -1;
    }

} // namespace WidgeCraft
