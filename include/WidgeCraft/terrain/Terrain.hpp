#pragma once

#include "WidgeCraft/model/Mesh.hpp"
#include "WidgeCraft/primitives/Shapes3D.hpp"
#include "WidgeCraft/terrain/HeightMap.hpp"

#include <memory>

namespace WidgeCraft {

    class Terrain {
    public:
        explicit Terrain(std::shared_ptr<HeightMap> heightMap);

        std::shared_ptr<HeightMap> getHeightMap() { return m_heightMap; }
        std::shared_ptr<const HeightMap> getHeightMap() const {
            return m_heightMap;
        }
        const Mesh3D& getMesh() const { return m_mesh; }

        void rebuildMesh();
        void draw(
            Shapes3D& shapes,
            const ShapeStyle3D& style,
            const Vec3& viewPosition = {},
            float drawDistance = 0.0f) const;

    private:
        std::shared_ptr<HeightMap> m_heightMap;
        Mesh3D m_mesh;
    };

} // namespace WidgeCraft
