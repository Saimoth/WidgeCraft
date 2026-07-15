#pragma once

#include "WidgeCraft/model/Mesh.hpp"
#include "WidgeCraft/primitives/Types.hpp"
#include "WidgeCraft/render/ShaderProgram.hpp"

#include <cstddef>
#include <vector>

namespace WidgeCraft {

    struct ShapeStyle3D {
        Color fillColor{ 0.35f, 0.60f, 0.90f, 1.0f };
        Color edgeColor{ 0.82f, 0.90f, 1.0f, 1.0f };
        float edgeThickness = 1.0f;
        bool fillVisible = true;
        bool edgeVisible = true;
    };

    class Shapes3D {
    public:
        Shapes3D();
        ~Shapes3D();

        Shapes3D(const Shapes3D&) = delete;
        Shapes3D& operator=(const Shapes3D&) = delete;
        Shapes3D(Shapes3D&& other) noexcept;
        Shapes3D& operator=(Shapes3D&& other) noexcept;

        void setViewProjection(const Mat4& viewProjection) {
            m_viewProjection = viewProjection;
        }
        const Mat4& getViewProjection() const {
            return m_viewProjection;
        }

        void beginFrame();
        void flush();
        void reserve(std::size_t triangleCount, std::size_t lineCount = 0);

        void drawLine(
            const Vec3& start,
            const Vec3& end,
            float thickness,
            const Color& color);
        void drawTriangle(
            const Vec3& a,
            const Vec3& b,
            const Vec3& c,
            const Color& color);
        void drawTriangle(
            const Vec3& a,
            const Vec3& b,
            const Vec3& c,
            const ShapeStyle3D& style);
        void drawQuad(
            const Vec3& a,
            const Vec3& b,
            const Vec3& c,
            const Vec3& d,
            const ShapeStyle3D& style);
        void drawBox(
            const Vec3& minimum,
            const Vec3& maximum,
            const ShapeStyle3D& style = ShapeStyle3D{});
        void drawCube(
            const Vec3& center,
            float size,
            const ShapeStyle3D& style = ShapeStyle3D{});
        void drawMesh(
            const Mesh3D& mesh,
            const Transform3D& transform = {},
            const ShapeStyle3D& style = ShapeStyle3D{});

        std::size_t getQueuedTriangleCount() const {
            return m_triangles.size() / 3;
        }
        std::size_t getQueuedLineCount() const {
            return m_lines.size();
        }
        std::size_t getQueuedVertexBytes() const {
            return m_triangles.size() * sizeof(Vertex)
                + m_lines.size() * sizeof(LineCommand);
        }

    private:
        struct Vertex {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;
        };

        struct LineCommand {
            Vertex start;
            Vertex end;
            float thickness = 1.0f;
        };

        static Vertex makeVertex(
            const Vec3& position,
            const Color& color);
        void appendTriangle(
            const Vec3& a,
            const Vec3& b,
            const Vec3& c,
            const Color& color);
        void appendBoxEdges(
            const Vec3* corners,
            float thickness,
            const Color& color);
        void cleanup();
        void moveFrom(Shapes3D&& other) noexcept;

        ShaderProgram m_shader;
        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        int m_uniformViewProjection = -1;
        Mat4 m_viewProjection = Mat4::identity();
        std::vector<Vertex> m_triangles;
        std::vector<LineCommand> m_lines;
        std::vector<Vertex> m_lineVertices;
    };

} // namespace WidgeCraft
