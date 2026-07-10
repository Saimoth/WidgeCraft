#pragma once

#include "WidgeCraft/Types.hpp"

namespace WidgeCraft {

    struct Ray {
        Vec3 origin{};
        Vec3 direction{ 0.0f, 0.0f, -1.0f };

        Vec3 pointAt(float distance) const {
            return origin + direction * distance;
        }
    };

    struct RayHit {
        float distance = 0.0f;
        Vec3 point{};
        Vec3 normal{};
        float u = 0.0f;
        float v = 0.0f;
    };

    struct Sphere {
        Vec3 center{};
        float radius = 1.0f;
    };

    struct AABB {
        Vec3 minimum{ -0.5f, -0.5f, -0.5f };
        Vec3 maximum{ 0.5f, 0.5f, 0.5f };
    };

    struct Plane {
        Vec3 normal{ 0.0f, 1.0f, 0.0f };
        float distance = 0.0f; // dot(normal, point) + distance = 0
    };

    struct Triangle {
        Vec3 a{};
        Vec3 b{};
        Vec3 c{};
    };

    bool invert(const Mat4& matrix, Mat4& inverseMatrix);

    bool raycast(const Ray& ray, const Sphere& sphere, RayHit& hit);
    bool raycast(const Ray& ray, const AABB& box, RayHit& hit);
    bool raycast(const Ray& ray, const Plane& plane, RayHit& hit);
    bool raycast(const Ray& ray, const Triangle& triangle, RayHit& hit, bool cullBackFaces = false);

    // screenX/screenY use WidgeCraft's bottom-left pixel coordinate system.
    Ray screenPointToRay(
        float screenX,
        float screenY,
        float viewportWidth,
        float viewportHeight,
        const Mat4& view,
        const Mat4& projection);

} // namespace WidgeCraft
