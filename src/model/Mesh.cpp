#include "WidgeCraft/model/Mesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace WidgeCraft {

    Vec3 Transform3D::transformPoint(const Vec3& point) const {
        Vec3 result{
            point.x * scale.x,
            point.y * scale.y,
            point.z * scale.z
        };

        const float cosX = std::cos(rotation.x);
        const float sinX = std::sin(rotation.x);
        result = {
            result.x,
            result.y * cosX - result.z * sinX,
            result.y * sinX + result.z * cosX
        };

        const float cosY = std::cos(rotation.y);
        const float sinY = std::sin(rotation.y);
        result = {
            result.x * cosY + result.z * sinY,
            result.y,
            -result.x * sinY + result.z * cosY
        };

        const float cosZ = std::cos(rotation.z);
        const float sinZ = std::sin(rotation.z);
        result = {
            result.x * cosZ - result.y * sinZ,
            result.x * sinZ + result.y * cosZ,
            result.z
        };
        return result + position;
    }

    bool Mesh3D::isValid() const {
        if (empty() || indices.size() % 3U != 0U) {
            return false;
        }
        return std::all_of(
            indices.begin(),
            indices.end(),
            [this](std::uint32_t index) {
                return index < vertices.size();
            });
    }

    AABB Mesh3D::getBounds() const {
        if (vertices.empty()) {
            return {};
        }

        const float maximum = std::numeric_limits<float>::max();
        Vec3 minimum{ maximum, maximum, maximum };
        Vec3 upper{ -maximum, -maximum, -maximum };
        for (const Vec3& vertex : vertices) {
            minimum.x = std::min(minimum.x, vertex.x);
            minimum.y = std::min(minimum.y, vertex.y);
            minimum.z = std::min(minimum.z, vertex.z);
            upper.x = std::max(upper.x, vertex.x);
            upper.y = std::max(upper.y, vertex.y);
            upper.z = std::max(upper.z, vertex.z);
        }
        return { minimum, upper };
    }

    bool Mesh2D::isValid() const {
        if (empty() || indices.size() % 3U != 0U) {
            return false;
        }
        return std::all_of(
            indices.begin(),
            indices.end(),
            [this](std::uint32_t index) {
                return index < vertices.size();
            });
    }

    Mesh3D makeBoxMesh(const Vec3& halfExtentsValue) {
        const Vec3 half{
            std::max(std::abs(halfExtentsValue.x), 0.001f),
            std::max(std::abs(halfExtentsValue.y), 0.001f),
            std::max(std::abs(halfExtentsValue.z), 0.001f)
        };

        Mesh3D mesh;
        mesh.vertices = {
            { -half.x, -half.y, -half.z },
            {  half.x, -half.y, -half.z },
            {  half.x,  half.y, -half.z },
            { -half.x,  half.y, -half.z },
            { -half.x, -half.y,  half.z },
            {  half.x, -half.y,  half.z },
            {  half.x,  half.y,  half.z },
            { -half.x,  half.y,  half.z }
        };
        mesh.indices = {
            0, 3, 2, 0, 2, 1,
            4, 5, 6, 4, 6, 7,
            0, 4, 7, 0, 7, 3,
            1, 2, 6, 1, 6, 5,
            0, 1, 5, 0, 5, 4,
            3, 7, 6, 3, 6, 2
        };
        return mesh;
    }

} // namespace WidgeCraft
