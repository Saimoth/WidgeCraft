#include "WidgeCraft/terrain/Terrain.hpp"

#include <cmath>
#include <stdexcept>

namespace WidgeCraft {

    Terrain::Terrain(std::shared_ptr<HeightMap> heightMap)
        : m_heightMap(std::move(heightMap)) {
        if (!m_heightMap || m_heightMap->empty()) {
            throw std::invalid_argument(
                "Terrain requires a populated heightmap");
        }
        rebuildMesh();
    }

    void Terrain::rebuildMesh() {
        m_mesh.vertices.clear();
        m_mesh.indices.clear();

        const std::size_t width = m_heightMap->getWidth();
        const std::size_t depth = m_heightMap->getDepth();
        const float cellSize = m_heightMap->getCellSize();
        const Vec3 origin = m_heightMap->getOrigin();

        m_mesh.vertices.reserve(width * depth);
        for (std::size_t z = 0; z < depth; ++z) {
            for (std::size_t x = 0; x < width; ++x) {
                m_mesh.vertices.push_back({
                    origin.x + static_cast<float>(x) * cellSize,
                    origin.y + m_heightMap->getHeight(x, z),
                    origin.z + static_cast<float>(z) * cellSize
                });
            }
        }

        m_mesh.indices.reserve((width - 1U) * (depth - 1U) * 6U);
        for (std::size_t z = 0; z + 1U < depth; ++z) {
            for (std::size_t x = 0; x + 1U < width; ++x) {
                const std::uint32_t a = static_cast<std::uint32_t>(
                    z * width + x);
                const std::uint32_t b = a + 1U;
                const std::uint32_t d = static_cast<std::uint32_t>(
                    (z + 1U) * width + x);
                const std::uint32_t c = d + 1U;

                if ((x + z) % 2U == 0U) {
                    m_mesh.indices.insert(
                        m_mesh.indices.end(),
                        { a, c, b, a, d, c });
                } else {
                    m_mesh.indices.insert(
                        m_mesh.indices.end(),
                        { a, d, b, b, d, c });
                }
            }
        }
    }

    void Terrain::draw(
        Shapes3D& shapes,
        const ShapeStyle3D& style,
        const Vec3& viewPosition,
        float drawDistance) const {

        if (drawDistance <= 0.0f) {
            shapes.drawMesh(m_mesh, {}, style);
            return;
        }

        const float distanceSquared = drawDistance * drawDistance;
        for (std::size_t index = 0;
             index + 2U < m_mesh.indices.size();
             index += 3U) {
            const Vec3& a = m_mesh.vertices[m_mesh.indices[index]];
            const Vec3& b = m_mesh.vertices[m_mesh.indices[index + 1U]];
            const Vec3& c = m_mesh.vertices[m_mesh.indices[index + 2U]];
            const Vec3 center = (a + b + c) / 3.0f;
            const float dx = center.x - viewPosition.x;
            const float dz = center.z - viewPosition.z;
            if (dx * dx + dz * dz <= distanceSquared) {
                shapes.drawTriangle(a, b, c, style);
            }
        }
    }

} // namespace WidgeCraft
