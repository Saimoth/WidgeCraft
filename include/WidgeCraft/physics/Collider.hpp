#pragma once

#include "WidgeCraft/primitives/Types.hpp"

#include <algorithm>

namespace WidgeCraft {

    struct BoxCollider {
        Vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
        Vec3 offset{};

        BoxCollider() = default;

        explicit BoxCollider(
            const Vec3& halfExtentsValue,
            const Vec3& offsetValue = {})
            : offset(offsetValue) {
            setHalfExtents(halfExtentsValue);
        }

        void setHalfExtents(const Vec3& value) {
            halfExtents = {
                std::max(value.x, 0.001f),
                std::max(value.y, 0.001f),
                std::max(value.z, 0.001f)
            };
        }
    };

} // namespace WidgeCraft
