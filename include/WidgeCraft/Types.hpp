#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace WidgeCraft {

    struct Color {
        float r = 0.1f;
        float g = 0.1f;
        float b = 0.1f;
        float a = 1.0f;
    };

    using Colour = Color;

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;

        constexpr Vec2 operator+(const Vec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
        constexpr Vec2 operator-(const Vec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
        constexpr Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
        constexpr Vec2 operator/(float scalar) const { return { x / scalar, y / scalar }; }
        Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
        Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    };

    inline float dot(const Vec2& lhs, const Vec2& rhs) {
        return lhs.x * rhs.x + lhs.y * rhs.y;
    }

    inline float lengthSquared(const Vec2& value) {
        return dot(value, value);
    }

    inline float length(const Vec2& value) {
        return std::sqrt(lengthSquared(value));
    }

    inline Vec2 normalized(const Vec2& value) {
        const float magnitude = length(value);
        return magnitude > 0.0f ? value / magnitude : Vec2{};
    }

    struct Vec3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        constexpr Vec3 operator+(const Vec3& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z }; }
        constexpr Vec3 operator-(const Vec3& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z }; }
        constexpr Vec3 operator-() const { return { -x, -y, -z }; }
        constexpr Vec3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
        constexpr Vec3 operator/(float scalar) const { return { x / scalar, y / scalar, z / scalar }; }
        Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
        Vec3& operator-=(const Vec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    };

    inline float dot(const Vec3& lhs, const Vec3& rhs) {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    inline Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
        return {
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        };
    }

    inline float lengthSquared(const Vec3& value) {
        return dot(value, value);
    }

    inline float length(const Vec3& value) {
        return std::sqrt(lengthSquared(value));
    }

    inline Vec3 normalized(const Vec3& value) {
        const float magnitude = length(value);
        return magnitude > 0.0f ? value / magnitude : Vec3{};
    }

    struct Vec4 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
    };

    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;

        constexpr float right() const { return x + width; }
        constexpr float top() const { return y + height; }

        constexpr bool contains(const Vec2& point) const {
            return point.x >= x && point.x <= right() && point.y >= y && point.y <= top();
        }
    };

    inline Rect intersect(const Rect& lhs, const Rect& rhs) {
        const float left = std::max(lhs.x, rhs.x);
        const float bottom = std::max(lhs.y, rhs.y);
        const float right = std::min(lhs.right(), rhs.right());
        const float top = std::min(lhs.top(), rhs.top());
        return { left, bottom, std::max(0.0f, right - left), std::max(0.0f, top - bottom) };
    }

    struct Mat4 {
        // OpenGL-compatible column-major storage.
        std::array<float, 16> values{};

        static constexpr Mat4 identity() {
            return { {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            } };
        }

        constexpr float& operator()(int row, int column) { return values[static_cast<size_t>(column * 4 + row)]; }
        constexpr float operator()(int row, int column) const { return values[static_cast<size_t>(column * 4 + row)]; }
    };

    inline Mat4 operator*(const Mat4& lhs, const Mat4& rhs) {
        Mat4 result{};
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                for (int index = 0; index < 4; ++index) {
                    result(row, column) += lhs(row, index) * rhs(index, column);
                }
            }
        }
        return result;
    }

    inline Vec4 operator*(const Mat4& matrix, const Vec4& vector) {
        return {
            matrix(0, 0) * vector.x + matrix(0, 1) * vector.y + matrix(0, 2) * vector.z + matrix(0, 3) * vector.w,
            matrix(1, 0) * vector.x + matrix(1, 1) * vector.y + matrix(1, 2) * vector.z + matrix(1, 3) * vector.w,
            matrix(2, 0) * vector.x + matrix(2, 1) * vector.y + matrix(2, 2) * vector.z + matrix(2, 3) * vector.w,
            matrix(3, 0) * vector.x + matrix(3, 1) * vector.y + matrix(3, 2) * vector.z + matrix(3, 3) * vector.w
        };
    }

    using Position = Vec2;
    using Size = Vec2;

    struct Offset {
        float dx = 0.0f;
        float dy = 0.0f;
    };

    enum class Anchor {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    namespace Colors {
        inline constexpr Color Transparent{ 0.0f, 0.0f, 0.0f, 0.0f };
        inline constexpr Color Black{ 0.0f, 0.0f, 0.0f, 1.0f };
        inline constexpr Color White{ 1.0f, 1.0f, 1.0f, 1.0f };
        inline constexpr Color Grey{ 0.5f, 0.5f, 0.5f, 1.0f };
        inline constexpr Color LightGrey{ 0.75f, 0.75f, 0.75f, 1.0f };
        inline constexpr Color DarkGrey{ 0.25f, 0.25f, 0.25f, 1.0f };
        inline constexpr Color Red{ 1.0f, 0.0f, 0.0f, 1.0f };
        inline constexpr Color Green{ 0.0f, 1.0f, 0.0f, 1.0f };
        inline constexpr Color Blue{ 0.0f, 0.0f, 1.0f, 1.0f };
        inline constexpr Color Yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
        inline constexpr Color Cyan{ 0.0f, 1.0f, 1.0f, 1.0f };
        inline constexpr Color Magenta{ 1.0f, 0.0f, 1.0f, 1.0f };
        inline constexpr Color Orange{ 1.0f, 0.5f, 0.0f, 1.0f };
        inline constexpr Color Purple{ 0.5f, 0.0f, 0.5f, 1.0f };
    } // namespace Colors

    namespace Colours {
        using namespace Colors;
    } // namespace Colours

} // namespace WidgeCraft
