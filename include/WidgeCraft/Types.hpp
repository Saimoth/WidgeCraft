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

