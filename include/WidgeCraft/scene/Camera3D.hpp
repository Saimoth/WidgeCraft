#pragma once

#include "WidgeCraft/primitives/Types.hpp"

#include <algorithm>
#include <cmath>

namespace WidgeCraft {

    enum class CameraProjection {
        Perspective,
        Orthographic
    };

    class Camera3D {
    public:
        void setPosition(const Vec3& position) { m_position = position; }
        Vec3 getPosition() const { return m_position; }

        void setTarget(const Vec3& target) { m_target = target; }
        Vec3 getTarget() const { return m_target; }

        void setUp(const Vec3& up) {
            m_up = lengthSquared(up) > 1.0e-12f
                ? normalized(up)
                : Vec3{ 0.0f, 1.0f, 0.0f };
        }
        Vec3 getUp() const { return m_up; }

        void setPerspective(
            float verticalFieldOfViewDegrees,
            float nearPlane = 0.1f,
            float farPlane = 1000.0f) {

            m_projection = CameraProjection::Perspective;
            m_verticalFieldOfViewDegrees = std::clamp(
                verticalFieldOfViewDegrees,
                1.0f,
                179.0f);
            setDepthRange(nearPlane, farPlane);
        }

        void setOrthographic(
            float visibleHeight,
            float nearPlane = 0.1f,
            float farPlane = 1000.0f) {

            m_projection = CameraProjection::Orthographic;
            m_orthographicHeight = std::max(visibleHeight, 0.0001f);
            setDepthRange(nearPlane, farPlane);
        }

        CameraProjection getProjectionType() const { return m_projection; }

        void setDepthRange(float nearPlane, float farPlane) {
            m_nearPlane = std::max(nearPlane, 0.0001f);
            m_farPlane = std::max(farPlane, m_nearPlane + 0.0001f);
        }

        float getNearPlane() const { return m_nearPlane; }
        float getFarPlane() const { return m_farPlane; }
        float getVerticalFieldOfViewDegrees() const {
            return m_verticalFieldOfViewDegrees;
        }
        float getOrthographicHeight() const { return m_orthographicHeight; }

        Mat4 getViewMatrix() const {
            Vec3 forward = m_target - m_position;
            if (lengthSquared(forward) <= 1.0e-12f) {
                forward = { 0.0f, 0.0f, -1.0f };
            } else {
                forward = normalized(forward);
            }

            Vec3 side = cross(forward, m_up);
            if (lengthSquared(side) <= 1.0e-12f) {
                const Vec3 fallbackUp =
                    std::abs(forward.y) < 0.99f
                    ? Vec3{ 0.0f, 1.0f, 0.0f }
                    : Vec3{ 0.0f, 0.0f, 1.0f };
                side = cross(forward, fallbackUp);
            }
            side = normalized(side);
            const Vec3 correctedUp = cross(side, forward);

            Mat4 view = Mat4::identity();
            view(0, 0) = side.x;
            view(0, 1) = side.y;
            view(0, 2) = side.z;
            view(0, 3) = -dot(side, m_position);

            view(1, 0) = correctedUp.x;
            view(1, 1) = correctedUp.y;
            view(1, 2) = correctedUp.z;
            view(1, 3) = -dot(correctedUp, m_position);

            view(2, 0) = -forward.x;
            view(2, 1) = -forward.y;
            view(2, 2) = -forward.z;
            view(2, 3) = dot(forward, m_position);
            return view;
        }

        Mat4 getProjectionMatrix(float aspectRatio) const {
            const float aspect = std::max(aspectRatio, 0.0001f);

            if (m_projection == CameraProjection::Orthographic) {
                const float halfHeight = m_orthographicHeight * 0.5f;
                const float halfWidth = halfHeight * aspect;
                const float left = -halfWidth;
                const float right = halfWidth;
                const float bottom = -halfHeight;
                const float top = halfHeight;

                Mat4 projection = Mat4::identity();
                projection(0, 0) = 2.0f / (right - left);
                projection(1, 1) = 2.0f / (top - bottom);
                projection(2, 2) = -2.0f / (m_farPlane - m_nearPlane);
                projection(0, 3) = -(right + left) / (right - left);
                projection(1, 3) = -(top + bottom) / (top - bottom);
                projection(2, 3) =
                    -(m_farPlane + m_nearPlane)
                    / (m_farPlane - m_nearPlane);
                return projection;
            }

            constexpr float pi = 3.14159265358979323846f;
            const float radians =
                m_verticalFieldOfViewDegrees * (pi / 180.0f);
            const float focalLength = 1.0f / std::tan(radians * 0.5f);

            Mat4 projection{};
            projection(0, 0) = focalLength / aspect;
            projection(1, 1) = focalLength;
            projection(2, 2) =
                (m_farPlane + m_nearPlane)
                / (m_nearPlane - m_farPlane);
            projection(2, 3) =
                (2.0f * m_farPlane * m_nearPlane)
                / (m_nearPlane - m_farPlane);
            projection(3, 2) = -1.0f;
            return projection;
        }

        Mat4 getViewProjection(float aspectRatio) const {
            return getProjectionMatrix(aspectRatio) * getViewMatrix();
        }

    private:
        Vec3 m_position{ 0.0f, 0.0f, 5.0f };
        Vec3 m_target{};
        Vec3 m_up{ 0.0f, 1.0f, 0.0f };
        CameraProjection m_projection = CameraProjection::Perspective;
        float m_verticalFieldOfViewDegrees = 60.0f;
        float m_orthographicHeight = 10.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 1000.0f;
    };

} // namespace WidgeCraft
