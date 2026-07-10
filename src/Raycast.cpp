#include "WidgeCraft/Raycast.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace WidgeCraft {

    namespace {
        constexpr float kEpsilon = 1.0e-6f;

        Vec3 divideByW(const Vec4& value) {
            if (std::abs(value.w) <= kEpsilon) {
                return { value.x, value.y, value.z };
            }
            return { value.x / value.w, value.y / value.w, value.z / value.w };
        }
    } // namespace

    bool invert(const Mat4& matrix, Mat4& inverseMatrix) {
        std::array<std::array<float, 8>, 4> augmented{};

        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                augmented[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = matrix(row, column);
            }
            augmented[static_cast<std::size_t>(row)][static_cast<std::size_t>(row + 4)] = 1.0f;
        }

        for (int pivotColumn = 0; pivotColumn < 4; ++pivotColumn) {
            int pivotRow = pivotColumn;
            float pivotMagnitude = std::abs(augmented[static_cast<std::size_t>(pivotRow)][static_cast<std::size_t>(pivotColumn)]);

            for (int row = pivotColumn + 1; row < 4; ++row) {
                const float candidate = std::abs(augmented[static_cast<std::size_t>(row)][static_cast<std::size_t>(pivotColumn)]);
                if (candidate > pivotMagnitude) {
                    pivotMagnitude = candidate;
                    pivotRow = row;
                }
            }

            if (pivotMagnitude <= kEpsilon) {
                inverseMatrix = Mat4::identity();
                return false;
            }

            if (pivotRow != pivotColumn) {
                std::swap(augmented[static_cast<std::size_t>(pivotRow)], augmented[static_cast<std::size_t>(pivotColumn)]);
            }

            const float pivot = augmented[static_cast<std::size_t>(pivotColumn)][static_cast<std::size_t>(pivotColumn)];
            for (float& value : augmented[static_cast<std::size_t>(pivotColumn)]) {
                value /= pivot;
            }

            for (int row = 0; row < 4; ++row) {
                if (row == pivotColumn) {
                    continue;
                }

                const float factor = augmented[static_cast<std::size_t>(row)][static_cast<std::size_t>(pivotColumn)];
                for (int column = 0; column < 8; ++column) {
                    augmented[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] -=
                        factor * augmented[static_cast<std::size_t>(pivotColumn)][static_cast<std::size_t>(column)];
                }
            }
        }

        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                inverseMatrix(row, column) = augmented[static_cast<std::size_t>(row)][static_cast<std::size_t>(column + 4)];
            }
        }
        return true;
    }

    bool raycast(const Ray& ray, const Sphere& sphere, RayHit& hit) {
        if (sphere.radius <= 0.0f) {
            return false;
        }

        const Vec3 offset = ray.origin - sphere.center;
        const float a = dot(ray.direction, ray.direction);
        if (a <= kEpsilon) {
            return false;
        }

        const float halfB = dot(offset, ray.direction);
        const float c = dot(offset, offset) - sphere.radius * sphere.radius;
        const float discriminant = halfB * halfB - a * c;
        if (discriminant < 0.0f) {
            return false;
        }

        const float root = std::sqrt(discriminant);
        float distance = (-halfB - root) / a;
        if (distance < 0.0f) {
            distance = (-halfB + root) / a;
        }
        if (distance < 0.0f) {
            return false;
        }

        hit.distance = distance;
        hit.point = ray.pointAt(distance);
        hit.normal = normalized(hit.point - sphere.center);
        hit.u = 0.0f;
        hit.v = 0.0f;
        return true;
    }

    bool raycast(const Ray& ray, const AABB& box, RayHit& hit) {
        float nearDistance = 0.0f;
        float farDistance = std::numeric_limits<float>::max();
        Vec3 nearNormal{};
        Vec3 farNormal{};

        const float origins[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
        const float directions[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
        const float minimums[3] = { box.minimum.x, box.minimum.y, box.minimum.z };
        const float maximums[3] = { box.maximum.x, box.maximum.y, box.maximum.z };

        for (int axis = 0; axis < 3; ++axis) {
            if (std::abs(directions[axis]) <= kEpsilon) {
                if (origins[axis] < minimums[axis] || origins[axis] > maximums[axis]) {
                    return false;
                }
                continue;
            }

            float axisNear = (minimums[axis] - origins[axis]) / directions[axis];
            float axisFar = (maximums[axis] - origins[axis]) / directions[axis];

            Vec3 negativeNormal{};
            Vec3 positiveNormal{};
            if (axis == 0) {
                negativeNormal = { -1.0f, 0.0f, 0.0f };
                positiveNormal = { 1.0f, 0.0f, 0.0f };
            } else if (axis == 1) {
                negativeNormal = { 0.0f, -1.0f, 0.0f };
                positiveNormal = { 0.0f, 1.0f, 0.0f };
            } else {
                negativeNormal = { 0.0f, 0.0f, -1.0f };
                positiveNormal = { 0.0f, 0.0f, 1.0f };
            }

            Vec3 axisNearNormal = negativeNormal;
            Vec3 axisFarNormal = positiveNormal;
            if (axisNear > axisFar) {
                std::swap(axisNear, axisFar);
                std::swap(axisNearNormal, axisFarNormal);
            }

            if (axisNear > nearDistance) {
                nearDistance = axisNear;
                nearNormal = axisNearNormal;
            }
            if (axisFar < farDistance) {
                farDistance = axisFar;
                farNormal = axisFarNormal;
            }

            if (nearDistance > farDistance) {
                return false;
            }
        }

        const bool startedInside = nearDistance <= kEpsilon;
        const float distance = startedInside ? farDistance : nearDistance;
        if (distance < 0.0f || !std::isfinite(distance)) {
            return false;
        }

        hit.distance = distance;
        hit.point = ray.pointAt(distance);
        hit.normal = startedInside ? farNormal : nearNormal;
        hit.u = 0.0f;
        hit.v = 0.0f;
        return true;
    }

    bool raycast(const Ray& ray, const Plane& plane, RayHit& hit) {
        const Vec3 normal = normalized(plane.normal);
        if (lengthSquared(normal) <= kEpsilon) {
            return false;
        }

        const float denominator = dot(normal, ray.direction);
        if (std::abs(denominator) <= kEpsilon) {
            return false;
        }

        const float distance = -(dot(normal, ray.origin) + plane.distance) / denominator;
        if (distance < 0.0f) {
            return false;
        }

        hit.distance = distance;
        hit.point = ray.pointAt(distance);
        hit.normal = denominator < 0.0f ? normal : -normal;
        hit.u = 0.0f;
        hit.v = 0.0f;
        return true;
    }

    bool raycast(const Ray& ray, const Triangle& triangle, RayHit& hit, bool cullBackFaces) {
        const Vec3 edge1 = triangle.b - triangle.a;
        const Vec3 edge2 = triangle.c - triangle.a;
        const Vec3 p = cross(ray.direction, edge2);
        const float determinant = dot(edge1, p);

        if (cullBackFaces) {
            if (determinant <= kEpsilon) {
                return false;
            }
        } else if (std::abs(determinant) <= kEpsilon) {
            return false;
        }

        const float inverseDeterminant = 1.0f / determinant;
        const Vec3 offset = ray.origin - triangle.a;
        const float u = dot(offset, p) * inverseDeterminant;
        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        const Vec3 q = cross(offset, edge1);
        const float v = dot(ray.direction, q) * inverseDeterminant;
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        const float distance = dot(edge2, q) * inverseDeterminant;
        if (distance < 0.0f) {
            return false;
        }

        Vec3 normal = normalized(cross(edge1, edge2));
        if (dot(normal, ray.direction) > 0.0f) {
            normal = -normal;
        }

        hit.distance = distance;
        hit.point = ray.pointAt(distance);
        hit.normal = normal;
        hit.u = u;
        hit.v = v;
        return true;
    }

    Ray screenPointToRay(
        float screenX,
        float screenY,
        float viewportWidth,
        float viewportHeight,
        const Mat4& view,
        const Mat4& projection) {

        if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
            return {};
        }

        const float normalizedX = (screenX / viewportWidth) * 2.0f - 1.0f;
        const float normalizedY = (screenY / viewportHeight) * 2.0f - 1.0f;

        Mat4 inverseViewProjection{};
        if (!invert(projection * view, inverseViewProjection)) {
            return {};
        }

        const Vec3 nearPoint = divideByW(inverseViewProjection * Vec4{ normalizedX, normalizedY, -1.0f, 1.0f });
        const Vec3 farPoint = divideByW(inverseViewProjection * Vec4{ normalizedX, normalizedY, 1.0f, 1.0f });

        return { nearPoint, normalized(farPoint - nearPoint) };
    }

} // namespace WidgeCraft
