#pragma once

#include "WidgeCraft/primitives/Types.hpp"
#include "WidgeCraft/scene/Raycast.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace WidgeCraft {

    struct Transform3D {
        Vec3 position{};
        // Euler angles in radians: pitch (x), yaw (y), roll (z).
        Vec3 rotation{};
        Vec3 scale{ 1.0f, 1.0f, 1.0f };

        Vec3 transformPoint(const Vec3& point) const;
    };

    struct Mesh3D {
        std::vector<Vec3> vertices;
        std::vector<std::uint32_t> indices;

        bool empty() const {
            return vertices.empty() || indices.size() < 3;
        }
        bool isValid() const;
        std::size_t getTriangleCount() const {
            return indices.size() / 3U;
        }
        std::size_t getMemoryUsageBytes() const {
            return vertices.capacity() * sizeof(Vec3)
                + indices.capacity() * sizeof(std::uint32_t);
        }
        AABB getBounds() const;
    };

    struct Mesh2D {
        std::vector<Vec2> vertices;
        std::vector<std::uint32_t> indices;

        bool empty() const {
            return vertices.empty() || indices.size() < 3;
        }
        bool isValid() const;
        std::size_t getTriangleCount() const {
            return indices.size() / 3U;
        }
    };

    Mesh3D makeBoxMesh(const Vec3& halfExtents);

} // namespace WidgeCraft
