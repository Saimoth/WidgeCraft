#pragma once

namespace WidgeCraft {

    struct Color {
        float r = 0.1f;
        float g = 0.1f;
        float b = 0.1f;
        float a = 1.0f;
    };

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;
    };

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

    using Colour = Color;

} // namespace WidgeCraft

