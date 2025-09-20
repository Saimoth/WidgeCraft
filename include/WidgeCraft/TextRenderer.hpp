#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct stbtt_fontinfo;

namespace WidgeCraft {

    class TextRenderer {
    public:
        struct Color {
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
        };

        TextRenderer(int screenWidth, int screenHeight, const std::string& fontPath, float fontPixelHeight = 48.0f);
        ~TextRenderer();

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;
        TextRenderer(TextRenderer&& other) noexcept;
        TextRenderer& operator=(TextRenderer&& other) noexcept;

        void setScreenSize(int width, int height);
        void renderText(const std::string& text, float x, float y, float scale = 1.0f, Color color = Color{1.0f, 1.0f, 1.0f});

        float getLineHeight() const { return m_lineHeight; }
        float getAscent() const { return m_ascent; }

    private:
        struct Glyph {
            float advance;
            float xOffset;
            float yOffset;
            float width;
            float height;
            float u0;
            float v0;
            float u1;
            float v1;
            int glyphIndex;
        };

        void cleanup();
        void moveFrom(TextRenderer&& other) noexcept;

        void createShader();
        void createBuffers();
        void uploadAtlasTexture();
        void updateProjection();

        bool loadFont(const std::string& fontPath);
        void buildAtlas(float pixelHeight);

        std::unordered_map<char, Glyph> m_glyphs;
        std::vector<unsigned char> m_atlasData;
        int m_atlasWidth = 0;
        int m_atlasHeight = 0;

        unsigned int m_texture = 0;
        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_shader = 0;

        std::unique_ptr<stbtt_fontinfo> m_fontInfo;
        std::vector<unsigned char> m_fontBuffer;

        float m_scale = 1.0f;
        float m_ascent = 0.0f;
        float m_descent = 0.0f;
        float m_lineHeight = 0.0f;

        int m_screenWidth = 0;
        int m_screenHeight = 0;

        float m_edgeValue = 0.0f;
        float m_smoothing = 0.0f;

        int m_uniformProjection = -1;
        int m_uniformTextColor = -1;
        int m_uniformEdgeValue = -1;
        int m_uniformSmoothing = -1;

        std::array<float, 16> m_projection{};

        const Glyph* m_fallbackGlyph = nullptr;
    };

} // namespace WidgeCraft
