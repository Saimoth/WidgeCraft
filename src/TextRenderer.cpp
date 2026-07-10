#include "WidgeCraft/TextRenderer.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace WidgeCraft {

    namespace {
        constexpr int kAtlasSize = 2048;
        constexpr int kPadding = 8;
        constexpr unsigned char kOnEdgeValue = 180;
        constexpr float kPixelDistanceScale = 64.0f;

        constexpr std::string_view kVertexShader = R"(
            #version 330 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            layout (location = 2) in vec4 aColor;

            uniform mat4 uProjection;

            out vec2 vTexCoord;
            out vec4 vColor;

            void main() {
                gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
                vTexCoord = aTexCoord;
                vColor = aColor;
            }
        )";

        constexpr std::string_view kFragmentShader = R"(
            #version 330 core
            in vec2 vTexCoord;
            in vec4 vColor;

            uniform sampler2D uTexture;
            uniform float uEdgeValue;
            uniform float uSoftness;

            out vec4 fragColor;

            void main() {
                float signedDistance = texture(uTexture, vTexCoord).r;
                float antialiasWidth = max(fwidth(signedDistance) * uSoftness, 0.00075);
                float alpha = smoothstep(
                    uEdgeValue - antialiasWidth,
                    uEdgeValue + antialiasWidth,
                    signedDistance);
                fragColor = vec4(vColor.rgb, vColor.a * alpha);
            }
        )";

        GLuint compileShader(GLenum type, std::string_view source) {
            const GLuint shader = glCreateShader(type);
            const char* data = source.data();
            const GLint length = static_cast<GLint>(source.size());
            glShaderSource(shader, 1, &data, &length);
            glCompileShader(shader);

            GLint status = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE) {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                glDeleteShader(shader);
                throw std::runtime_error("SDF text shader compilation failed: " + log);
            }
            return shader;
        }

        GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
            const GLuint program = glCreateProgram();
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);

            GLint status = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &status);
            if (status == GL_FALSE) {
                GLint logLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
                glGetProgramInfoLog(program, logLength, nullptr, log.data());
                glDeleteProgram(program);
                throw std::runtime_error("SDF text shader linking failed: " + log);
            }
            return program;
        }

        std::vector<char32_t> decodeUtf8(std::string_view text) {
            std::vector<char32_t> output;
            output.reserve(text.size());

            std::size_t index = 0;
            while (index < text.size()) {
                const auto first = static_cast<unsigned char>(text[index]);
                if (first < 0x80U) {
                    output.push_back(static_cast<char32_t>(first));
                    ++index;
                    continue;
                }

                int continuationCount = 0;
                char32_t codepoint = 0;
                if ((first & 0xE0U) == 0xC0U) {
                    continuationCount = 1;
                    codepoint = first & 0x1FU;
                } else if ((first & 0xF0U) == 0xE0U) {
                    continuationCount = 2;
                    codepoint = first & 0x0FU;
                } else if ((first & 0xF8U) == 0xF0U) {
                    continuationCount = 3;
                    codepoint = first & 0x07U;
                } else {
                    output.push_back(U'\uFFFD');
                    ++index;
                    continue;
                }

                if (index + static_cast<std::size_t>(continuationCount) >= text.size()) {
                    output.push_back(U'\uFFFD');
                    break;
                }

                bool valid = true;
                for (int continuation = 1; continuation <= continuationCount; ++continuation) {
                    const auto value = static_cast<unsigned char>(text[index + static_cast<std::size_t>(continuation)]);
                    if ((value & 0xC0U) != 0x80U) {
                        valid = false;
                        break;
                    }
                    codepoint = (codepoint << 6U) | (value & 0x3FU);
                }

                const bool overlong =
                    (continuationCount == 1 && codepoint < 0x80U)
                    || (continuationCount == 2 && codepoint < 0x800U)
                    || (continuationCount == 3 && codepoint < 0x10000U);
                const bool invalidRange = codepoint > 0x10FFFFU || (codepoint >= 0xD800U && codepoint <= 0xDFFFU);

                if (!valid || overlong || invalidRange) {
                    output.push_back(U'\uFFFD');
                    ++index;
                    continue;
                }

                output.push_back(codepoint);
                index += static_cast<std::size_t>(continuationCount + 1);
            }
            return output;
        }

        std::vector<char32_t> defaultCodepoints() {
            std::vector<char32_t> codepoints;
            codepoints.reserve(256);
            for (char32_t codepoint = 32; codepoint <= 255; ++codepoint) {
                codepoints.push_back(codepoint);
            }

            const char32_t extras[] = {
                U'\u2013', U'\u2014', U'\u2018', U'\u2019', U'\u201C', U'\u201D',
                U'\u2022', U'\u2026', U'\u20AC', U'\u2190', U'\u2191', U'\u2192',
                U'\u2193', U'\u2713', U'\u2715', U'\uFFFD'
            };
            codepoints.insert(codepoints.end(), std::begin(extras), std::end(extras));
            return codepoints;
        }

    } // namespace

    TextRenderer::TextRenderer(int screenWidth, int screenHeight, const std::string& fontPath, float fontPixelHeight)
        : m_fontInfo(std::make_unique<stbtt_fontinfo>())
        , m_screenWidth(screenWidth)
        , m_screenHeight(screenHeight) {

        if (!loadFont(fontPath)) {
            throw std::runtime_error("Failed to load font: " + fontPath);
        }

        buildAtlas(fontPixelHeight);
        createShader();
        createBuffers();
        uploadAtlasTexture();
        updateProjection();
        m_vertices.reserve(4096);
    }

    TextRenderer::~TextRenderer() {
        cleanup();
    }

    TextRenderer::TextRenderer(TextRenderer&& other) noexcept {
        moveFrom(std::move(other));
    }

    TextRenderer& TextRenderer::operator=(TextRenderer&& other) noexcept {
        if (this != &other) {
            cleanup();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void TextRenderer::setScreenSize(int width, int height) {
        if (m_screenWidth == width && m_screenHeight == height) {
            return;
        }
        m_screenWidth = width;
        m_screenHeight = height;
        updateProjection();
    }

    void TextRenderer::beginFrame() {
        m_vertices.clear();
    }

    void TextRenderer::flush() {
        if (m_vertices.empty() || m_shader == 0 || m_texture == 0) {
            m_vertices.clear();
            return;
        }

        glUseProgram(m_shader);
        glUniformMatrix4fv(m_uniformProjection, 1, GL_FALSE, m_projection.values.data());
        glUniform1f(m_uniformEdgeValue, m_edgeValue);
        glUniform1f(m_uniformSoftness, m_softness);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
            m_vertices.data(),
            GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        m_vertices.clear();
    }

    void TextRenderer::renderText(const std::string& text, float x, float y, float sizePixels, Color color) {
        if (text.empty() || m_basePixelHeight <= 0.0f) {
            return;
        }
        if (sizePixels <= 0.0f) {
            sizePixels = m_basePixelHeight;
        }
        if (sizePixels <= 0.0f) {
            return;
        }

        const std::vector<char32_t> codepoints = decodeUtf8(text);
        const float sizeScale = sizePixels / m_basePixelHeight;
        float cursorX = x;
        float cursorY = y;

        for (std::size_t index = 0; index < codepoints.size(); ++index) {
            const char32_t codepoint = codepoints[index];
            if (codepoint == U'\r') {
                continue;
            }
            if (codepoint == U'\n') {
                cursorX = x;
                cursorY -= m_lineHeight * sizeScale;
                continue;
            }

            const Glyph* glyph = findGlyph(codepoint);
            if (!glyph) {
                continue;
            }

            const float xPosition = cursorX + glyph->xOffset * sizeScale;
            const float yPosition = cursorY - (glyph->yOffset + glyph->height) * sizeScale;
            const float width = glyph->width * sizeScale;
            const float height = glyph->height * sizeScale;

            if (width > 0.0f && height > 0.0f) {
                const Vertex bottomLeft{ xPosition, yPosition, glyph->u0, glyph->v0, color.r, color.g, color.b, color.a };
                const Vertex bottomRight{ xPosition + width, yPosition, glyph->u1, glyph->v0, color.r, color.g, color.b, color.a };
                const Vertex topRight{ xPosition + width, yPosition + height, glyph->u1, glyph->v1, color.r, color.g, color.b, color.a };
                const Vertex topLeft{ xPosition, yPosition + height, glyph->u0, glyph->v1, color.r, color.g, color.b, color.a };

                m_vertices.push_back(bottomLeft);
                m_vertices.push_back(bottomRight);
                m_vertices.push_back(topRight);
                m_vertices.push_back(bottomLeft);
                m_vertices.push_back(topRight);
                m_vertices.push_back(topLeft);
            }

            cursorX += glyph->advance * sizeScale;

            if (index + 1 < codepoints.size() && m_fontInfo) {
                const char32_t nextCodepoint = codepoints[index + 1];
                if (nextCodepoint != U'\n' && nextCodepoint != U'\r') {
                    const Glyph* nextGlyph = findGlyph(nextCodepoint);
                    if (nextGlyph) {
                        const int kerning = stbtt_GetGlyphKernAdvance(
                            m_fontInfo.get(),
                            glyph->glyphIndex,
                            nextGlyph->glyphIndex);
                        cursorX += static_cast<float>(kerning) * m_scale * sizeScale;
                    }
                }
            }
        }
    }

    void TextRenderer::renderTextCentered(const std::string& text, float centerX, float centerY, float sizePixels, Color color) {
        const auto bounds = measureTextBounds(text, sizePixels);
        if (!bounds) {
            return;
        }

        const float x = centerX - (bounds->minX + bounds->maxX) * 0.5f;
        const float y = centerY - (bounds->minY + bounds->maxY) * 0.5f;
        renderText(text, x, y, sizePixels, color);
    }

    std::optional<TextRenderer::TextBounds> TextRenderer::measureTextBounds(const std::string& text, float sizePixels) const {
        if (text.empty() || m_basePixelHeight <= 0.0f) {
            return std::nullopt;
        }
        if (sizePixels <= 0.0f) {
            sizePixels = m_basePixelHeight;
        }
        if (sizePixels <= 0.0f) {
            return std::nullopt;
        }

        const std::vector<char32_t> codepoints = decodeUtf8(text);
        const float sizeScale = sizePixels / m_basePixelHeight;
        float cursorX = 0.0f;
        float cursorY = 0.0f;
        float lowestBaseline = 0.0f;
        float maximumLineWidth = 0.0f;
        bool hasGeometry = false;

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();

        for (std::size_t index = 0; index < codepoints.size(); ++index) {
            const char32_t codepoint = codepoints[index];
            if (codepoint == U'\r') {
                continue;
            }
            if (codepoint == U'\n') {
                maximumLineWidth = std::max(maximumLineWidth, cursorX);
                cursorX = 0.0f;
                cursorY -= m_lineHeight * sizeScale;
                lowestBaseline = std::min(lowestBaseline, cursorY);
                continue;
            }

            const Glyph* glyph = findGlyph(codepoint);
            if (!glyph) {
                continue;
            }

            const float xPosition = cursorX + glyph->xOffset * sizeScale;
            const float yPosition = cursorY - (glyph->yOffset + glyph->height) * sizeScale;
            const float width = glyph->width * sizeScale;
            const float height = glyph->height * sizeScale;

            if (width > 0.0f && height > 0.0f) {
                hasGeometry = true;
                minX = std::min(minX, xPosition);
                minY = std::min(minY, yPosition);
                maxX = std::max(maxX, xPosition + width);
                maxY = std::max(maxY, yPosition + height);
            }

            cursorX += glyph->advance * sizeScale;

            if (index + 1 < codepoints.size() && m_fontInfo) {
                const char32_t nextCodepoint = codepoints[index + 1];
                if (nextCodepoint != U'\n' && nextCodepoint != U'\r') {
                    const Glyph* nextGlyph = findGlyph(nextCodepoint);
                    if (nextGlyph) {
                        const int kerning = stbtt_GetGlyphKernAdvance(
                            m_fontInfo.get(),
                            glyph->glyphIndex,
                            nextGlyph->glyphIndex);
                        cursorX += static_cast<float>(kerning) * m_scale * sizeScale;
                    }
                }
            }

            maximumLineWidth = std::max(maximumLineWidth, cursorX);
        }

        maximumLineWidth = std::max(maximumLineWidth, cursorX);
        const float metricsTop = m_ascent * sizeScale;
        const float metricsBottom = lowestBaseline + m_descent * sizeScale;

        if (!hasGeometry) {
            minX = 0.0f;
            maxX = maximumLineWidth;
            minY = metricsBottom;
            maxY = metricsTop;
        } else {
            minX = std::min(minX, 0.0f);
            maxX = std::max(maxX, maximumLineWidth);
            minY = std::min(minY, metricsBottom);
            maxY = std::max(maxY, metricsTop);
        }

        return TextBounds{ minX, minY, maxX, maxY };
    }

    float TextRenderer::measureTextWidth(const std::string& text, float sizePixels) const {
        const auto bounds = measureTextBounds(text, sizePixels);
        return bounds ? bounds->width() : 0.0f;
    }

    void TextRenderer::cleanup() {
        if (m_texture != 0) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
        if (m_shader != 0) {
            glDeleteProgram(m_shader);
            m_shader = 0;
        }

        m_fontInfo.reset();
        m_glyphs.clear();
        m_atlasData.clear();
        m_fontBuffer.clear();
        m_vertices.clear();
        m_fallbackGlyph = nullptr;
    }

    void TextRenderer::moveFrom(TextRenderer&& other) noexcept {
        m_glyphs = std::move(other.m_glyphs);
        m_atlasData = std::move(other.m_atlasData);
        m_fontBuffer = std::move(other.m_fontBuffer);
        m_vertices = std::move(other.m_vertices);
        m_atlasWidth = other.m_atlasWidth;
        m_atlasHeight = other.m_atlasHeight;
        m_texture = other.m_texture;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_shader = other.m_shader;
        m_fontInfo = std::move(other.m_fontInfo);
        m_scale = other.m_scale;
        m_ascent = other.m_ascent;
        m_descent = other.m_descent;
        m_lineHeight = other.m_lineHeight;
        m_basePixelHeight = other.m_basePixelHeight;
        m_edgeValue = other.m_edgeValue;
        m_softness = other.m_softness;
        m_screenWidth = other.m_screenWidth;
        m_screenHeight = other.m_screenHeight;
        m_uniformProjection = other.m_uniformProjection;
        m_uniformEdgeValue = other.m_uniformEdgeValue;
        m_uniformSoftness = other.m_uniformSoftness;
        m_projection = other.m_projection;

        if (auto it = m_glyphs.find(U'\uFFFD'); it != m_glyphs.end()) {
            m_fallbackGlyph = &it->second;
        } else if (auto it = m_glyphs.find(U'?'); it != m_glyphs.end()) {
            m_fallbackGlyph = &it->second;
        }

        other.m_atlasWidth = 0;
        other.m_atlasHeight = 0;
        other.m_texture = 0;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_shader = 0;
        other.m_fallbackGlyph = nullptr;
        other.m_uniformProjection = -1;
        other.m_uniformEdgeValue = -1;
        other.m_uniformSoftness = -1;
    }

    bool TextRenderer::loadFont(const std::string& fontPath) {
        std::ifstream file(fontPath, std::ios::binary);
        if (!file) {
            return false;
        }

        file.seekg(0, std::ios::end);
        const std::streampos fileSize = file.tellg();
        if (fileSize <= 0) {
            return false;
        }
        file.seekg(0, std::ios::beg);

        m_fontBuffer.resize(static_cast<std::size_t>(fileSize));
        file.read(reinterpret_cast<char*>(m_fontBuffer.data()), static_cast<std::streamsize>(fileSize));
        if (!file) {
            return false;
        }

        return stbtt_InitFont(m_fontInfo.get(), m_fontBuffer.data(), 0) != 0;
    }

    void TextRenderer::buildAtlas(float pixelHeight) {
        if (!m_fontInfo) {
            throw std::runtime_error("Font data has not been loaded");
        }

        pixelHeight = std::clamp(pixelHeight, 16.0f, 256.0f);
        m_glyphs.clear();
        m_atlasWidth = kAtlasSize;
        m_atlasHeight = kAtlasSize;
        m_atlasData.assign(static_cast<std::size_t>(m_atlasWidth * m_atlasHeight), 0);
        m_scale = stbtt_ScaleForPixelHeight(m_fontInfo.get(), pixelHeight);

        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(m_fontInfo.get(), &ascent, &descent, &lineGap);
        m_ascent = static_cast<float>(ascent) * m_scale;
        m_descent = static_cast<float>(descent) * m_scale;
        m_lineHeight = static_cast<float>(ascent - descent + lineGap) * m_scale;
        m_basePixelHeight = m_ascent - m_descent;

        const std::vector<char32_t> requestedCodepoints = defaultCodepoints();
        std::unordered_set<char32_t> seen;
        seen.reserve(requestedCodepoints.size());
        m_glyphs.reserve(requestedCodepoints.size());

        int penX = kPadding;
        int penY = kPadding;
        int rowHeight = 0;

        for (const char32_t codepoint : requestedCodepoints) {
            if (!seen.insert(codepoint).second) {
                continue;
            }

            const int glyphIndex = stbtt_FindGlyphIndex(m_fontInfo.get(), static_cast<int>(codepoint));
            if (glyphIndex == 0 && codepoint != U'?') {
                continue;
            }

            int width = 0;
            int height = 0;
            int xOffset = 0;
            int yOffset = 0;
            unsigned char* sdf = nullptr;

            if (stbtt_IsGlyphEmpty(m_fontInfo.get(), glyphIndex) == 0) {
                sdf = stbtt_GetGlyphSDF(
                    m_fontInfo.get(),
                    m_scale,
                    glyphIndex,
                    kPadding,
                    kOnEdgeValue,
                    kPixelDistanceScale,
                    &width,
                    &height,
                    &xOffset,
                    &yOffset);
            }

            if (sdf && width > 0 && height > 0) {
                if (penX + width + kPadding > m_atlasWidth) {
                    penX = kPadding;
                    penY += rowHeight + kPadding;
                    rowHeight = 0;
                }
                if (penY + height + kPadding > m_atlasHeight) {
                    stbtt_FreeSDF(sdf, nullptr);
                    throw std::runtime_error("SDF font atlas is too small for the default character set");
                }

                for (int row = 0; row < height; ++row) {
                    unsigned char* destination = m_atlasData.data()
                        + static_cast<std::size_t>((penY + row) * m_atlasWidth + penX);
                    const unsigned char* source = sdf + static_cast<std::size_t>(row * width);
                    std::copy(source, source + width, destination);
                }
                rowHeight = std::max(rowHeight, height);
            }

            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetGlyphHMetrics(m_fontInfo.get(), glyphIndex, &advanceWidth, &leftSideBearing);
            (void)leftSideBearing;

            Glyph glyph{};
            glyph.advance = static_cast<float>(advanceWidth) * m_scale;
            glyph.xOffset = static_cast<float>(xOffset);
            glyph.yOffset = static_cast<float>(yOffset);
            glyph.width = static_cast<float>(width);
            glyph.height = static_cast<float>(height);
            glyph.u0 = width > 0 ? static_cast<float>(penX) / static_cast<float>(m_atlasWidth) : 0.0f;
            glyph.v0 = height > 0 ? static_cast<float>(penY + height) / static_cast<float>(m_atlasHeight) : 0.0f;
            glyph.u1 = width > 0 ? static_cast<float>(penX + width) / static_cast<float>(m_atlasWidth) : 0.0f;
            glyph.v1 = height > 0 ? static_cast<float>(penY) / static_cast<float>(m_atlasHeight) : 0.0f;
            glyph.glyphIndex = glyphIndex;
            m_glyphs.emplace(codepoint, glyph);

            if (sdf) {
                penX += width + kPadding;
                stbtt_FreeSDF(sdf, nullptr);
            }
        }

        if (auto it = m_glyphs.find(U'\uFFFD'); it != m_glyphs.end()) {
            m_fallbackGlyph = &it->second;
        } else if (auto it = m_glyphs.find(U'?'); it != m_glyphs.end()) {
            m_fallbackGlyph = &it->second;
        } else if (!m_glyphs.empty()) {
            m_fallbackGlyph = &m_glyphs.begin()->second;
        }

        m_edgeValue = static_cast<float>(kOnEdgeValue) / 255.0f;
        m_softness = 1.0f;
    }

    void TextRenderer::createShader() {
        const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShader);
        const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
        m_shader = linkProgram(vertexShader, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glUseProgram(m_shader);
        glUniform1i(glGetUniformLocation(m_shader, "uTexture"), 0);
        m_uniformProjection = glGetUniformLocation(m_shader, "uProjection");
        m_uniformEdgeValue = glGetUniformLocation(m_shader, "uEdgeValue");
        m_uniformSoftness = glGetUniformLocation(m_shader, "uSoftness");
        glUseProgram(0);
    }

    void TextRenderer::createBuffers() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, u)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void TextRenderer::uploadAtlasTexture() {
        if (m_atlasData.empty()) {
            throw std::runtime_error("SDF atlas data is empty");
        }

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R8,
            m_atlasWidth,
            m_atlasHeight,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            m_atlasData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void TextRenderer::updateProjection() {
        const float width = static_cast<float>(m_screenWidth);
        const float height = static_cast<float>(m_screenHeight);
        if (width <= 0.0f || height <= 0.0f) {
            m_projection = Mat4::identity();
            return;
        }

        m_projection = Mat4::identity();
        m_projection(0, 0) = 2.0f / width;
        m_projection(1, 1) = 2.0f / height;
        m_projection(2, 2) = -1.0f;
        m_projection(0, 3) = -1.0f;
        m_projection(1, 3) = -1.0f;
    }

    const TextRenderer::Glyph* TextRenderer::findGlyph(char32_t codepoint) const {
        if (const auto it = m_glyphs.find(codepoint); it != m_glyphs.end()) {
            return &it->second;
        }
        return m_fallbackGlyph;
    }

} // namespace WidgeCraft
