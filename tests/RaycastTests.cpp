#include "WidgeCraft/Raycast.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {
    bool nearlyEqual(float lhs, float rhs, float tolerance = 1.0e-4f) {
        return std::abs(lhs - rhs) <= tolerance;
    }
}

int main() {
    using namespace WidgeCraft;

    {
        const Ray ray{ { 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -1.0f } };
        const Sphere sphere{ { 0.0f, 0.0f, 0.0f }, 1.0f };
        RayHit hit{};
        assert(raycast(ray, sphere, hit));
        assert(nearlyEqual(hit.distance, 4.0f));
        assert(nearlyEqual(hit.point.z, 1.0f));
        assert(nearlyEqual(hit.normal.z, 1.0f));
    }

    {
        const Ray ray{ { -2.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
        const AABB box{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } };
        RayHit hit{};
        assert(raycast(ray, box, hit));
        assert(nearlyEqual(hit.distance, 1.0f));
        assert(nearlyEqual(hit.normal.x, -1.0f));
    }

    {
        const Ray ray{ { 0.0f, 2.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } };
        const Plane plane{ { 0.0f, 1.0f, 0.0f }, 0.0f };
        RayHit hit{};
        assert(raycast(ray, plane, hit));
        assert(nearlyEqual(hit.distance, 2.0f));
        assert(nearlyEqual(hit.point.y, 0.0f));
    }

    {
        const Ray ray{ { 0.25f, 0.25f, 1.0f }, { 0.0f, 0.0f, -1.0f } };
        const Triangle triangle{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
        RayHit hit{};
        assert(raycast(ray, triangle, hit));
        assert(nearlyEqual(hit.distance, 1.0f));
        assert(nearlyEqual(hit.u, 0.25f));
        assert(nearlyEqual(hit.v, 0.25f));
    }

    {
        const Mat4 identity = Mat4::identity();
        const Ray ray = screenPointToRay(400.0f, 300.0f, 800.0f, 600.0f, identity, identity);
        assert(nearlyEqual(ray.origin.x, 0.0f));
        assert(nearlyEqual(ray.origin.y, 0.0f));
        assert(nearlyEqual(ray.origin.z, -1.0f));
        assert(nearlyEqual(ray.direction.z, 1.0f));
    }

    std::cout << "Raycast tests passed\n";
    return 0;
}
