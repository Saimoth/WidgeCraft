#pragma once

#include "WidgeCraft/Types.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct stbtt_fontinfo;

namespace WidgeCraft {

    class TextRenderer {
    public:
        using Color = WidgeCraft::Color;

        struct TextBounds {
            float minX = 0.0f;
            float minY = 0.0f;
            float maxX = 0.0f;
            float maxY = 0.0f;

            float width() const { return maxX - minX; }
            float height() const { return maxY - minY; }
        };

        TextRenderer(int screenWidth, int screenHeight, const std::string& fontPath, float fontPixelHeight = 64.0f);
        ~TextRenderer();

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;
        TextRenderer(TextRenderer&& other) noexcept;
        TextRenderer& operator=(TextRenderer&& other) noexcept;

        void setScreenSize(int width, int height);
        void beginFrame();
        void flush();

        // x/y is the baseline origin in WidgeCraft's bottom-left coordinate system.
        void renderText(const std::string& text, float x, float y, float sizePixels = 0.0f, Color color = Colors::White);
        void renderTextCentered(const std::string& text, float centerX, float centerY, float sizePixels = 0.0f, Color color = Colors::White);

        std::optional<TextBounds> measureTextBounds(const std::string& text, float sizePixels = 0.0f) const;
        float measureTextWidth(const std::string& text, float sizePixels = 0.0f) const;

        float getLineHeight() const { return m_lineHeight; }
        float getAscent() const { return m_ascent; }
        float getDescent() const { return m_descent; }
        float getBasePixelHeight() const { return m_basePixelHeight; }
        std::size_t getQueuedGlyphCount() const { return m_vertices.size() / 6; }

    private:
        struct Glyph {
            float advance = 0.0f;
            float xOffset = 0.0f;
            float yOffset = 0.0f;
            float width = 0.0f;
            float height = 0.0f;
            float u0 = 0.0f;
            float v0 = 0.0f;
            float u1 = 0.0f;
            float v1 = 0.0f;
            int glyphIndex = 0;
        };

        struct Vertex {
            float x = 0.0f;
            float y = 0.0f;
            float u = 0.0f;
            float v = 0.0f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;
        };

        void cleanup();
        void moveFrom(TextRenderer&& other) noexcept;
        void createShader();
        void createBuffers();
        void uploadAtlasTexture();
        void updateProjection();
        bool loadFont(const std::string& fontPath);
        void buildAtlas(float pixelHeight);
        const Glyph* findGlyph(char32_t codepoint) const;

        std::unordered_map<char32_t, Glyph> m_glyphs;
        std::vector<unsigned char> m_atlasData;
        std::vector<unsigned char> m_fontBuffer;
        std::vector<Vertex> m_vertices;
        int m_atlasWidth = 0;
        int m_atlasHeight = 0;

        unsigned int m_texture = 0;
        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_shader = 0;

        std::unique_ptr<stbtt_fontinfo> m_fontInfo;
        const Glyph* m_fallbackGlyph = nullptr;

        float m_scale = 1.0f;
        float m_ascent = 0.0f;
        float m_descent = 0.0f;
        float m_lineHeight = 0.0f;
        float m_basePixelHeight = 0.0f;
        float m_edgeValue = 0.0f;
        float m_softness = 1.0f;

        int m_screenWidth = 0;
        int m_screenHeight = 0;
        int m_uniformProjection = -1;
        int m_uniformEdgeValue = -1;
        int m_uniformSoftness = -1;
        Mat4 m_projection = Mat4::identity();
    };

} // namespace WidgeCraft
