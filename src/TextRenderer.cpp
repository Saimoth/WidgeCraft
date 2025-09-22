#include "WidgeCraft/TextRenderer.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace WidgeCraft {

    namespace {
        std::filesystem::path resolveFontPath(const std::string& fontPath) {
            namespace fs = std::filesystem;

            if (fontPath.empty()) {
                return {};
            }

            fs::path requested(fontPath);

            std::error_code existsEc;
            if (requested.is_absolute()) {
                if (fs::exists(requested, existsEc) && !existsEc) {
                    return requested;
                }
                return {};
            }

            std::error_code currentEc;
            fs::path dir = fs::current_path(currentEc);
            if (currentEc) {
                return {};
            }

            while (true) {
                fs::path candidate = dir / requested;
                existsEc.clear();
                if (fs::exists(candidate, existsEc) && !existsEc) {
                    std::error_code absoluteEc;
                    fs::path absoluteCandidate = fs::absolute(candidate, absoluteEc);
                    if (!absoluteEc) {
                        return absoluteCandidate;
                    }
                    return candidate;
                }

                fs::path parent = dir.parent_path();
                if (parent.empty() || parent == dir) {
                    break;
                }
                dir = parent;
            }

            return {};
        }

        constexpr int kAtlasSize = 1024;
        constexpr int kFirstChar = 32;
        constexpr int kLastChar = 126;
        constexpr int kPadding = 6;
        constexpr unsigned char kOnEdgeValue = 180;
        constexpr float kPixelDistScale = 64.0f;

        constexpr std::string_view kVertexShaderSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;

            uniform mat4 uProjection;

            out vec2 vTexCoord;

            void main() {
                vTexCoord = aTexCoord;
                gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
            }
        )";

        constexpr std::string_view kFragmentShaderSrc = R"(
            #version 330 core
            in vec2 vTexCoord;

            uniform sampler2D uTexture;
            uniform vec3 uTextColor;
            uniform float uEdgeValue;
            uniform float uSmoothing;

            out vec4 FragColor;

            void main() {
                float distance = texture(uTexture, vTexCoord).r;
                float alpha = smoothstep(uEdgeValue - uSmoothing, uEdgeValue + uSmoothing, distance);
                FragColor = vec4(uTextColor, alpha);
            }
        )";

        GLuint compileShader(GLenum type, std::string_view source) {
            GLuint shader = glCreateShader(type);
            const char* src = source.data();
            GLint length = static_cast<GLint>(source.size());
            glShaderSource(shader, 1, &src, &length);
            glCompileShader(shader);

            GLint status = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE) {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(static_cast<size_t>(logLength), '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                glDeleteShader(shader);
                throw std::runtime_error("Shader compilation failed: " + log);
            }

            return shader;
        }

        GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
            GLuint program = glCreateProgram();
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);

            GLint status = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &status);
            if (status == GL_FALSE) {
                GLint logLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(static_cast<size_t>(logLength), '\0');
                glGetProgramInfoLog(program, logLength, nullptr, log.data());
                glDeleteProgram(program);
                throw std::runtime_error("Shader linking failed: " + log);
            }

            return program;
        }

    } // namespace

    TextRenderer::TextRenderer(int screenWidth, int screenHeight, const std::string& fontPath, float fontPixelHeight)
        : m_fontInfo(std::make_unique<stbtt_fontinfo>()),
          m_screenWidth(screenWidth),
          m_screenHeight(screenHeight) {
        if (!loadFont(fontPath)) {
            std::error_code ec;
            const auto base = std::filesystem::current_path(ec);
            if (!ec) {
                throw std::runtime_error(
                    "Failed to load font \"" + fontPath + "\" (starting search from \"" + base.string() + "\")");
            }
            throw std::runtime_error("Failed to load font: " + fontPath);
        }

        buildAtlas(fontPixelHeight);
        createShader();
        createBuffers();
        uploadAtlasTexture();
        updateProjection();
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
        m_fallbackGlyph = nullptr;
    }

    void TextRenderer::moveFrom(TextRenderer&& other) noexcept {
        m_texture = other.m_texture;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_shader = other.m_shader;
        m_fontInfo = std::move(other.m_fontInfo);
        m_fontBuffer = std::move(other.m_fontBuffer);
        m_glyphs = std::move(other.m_glyphs);
        m_atlasData = std::move(other.m_atlasData);
        m_atlasWidth = other.m_atlasWidth;
        m_atlasHeight = other.m_atlasHeight;
        m_scale = other.m_scale;
        m_ascent = other.m_ascent;
        m_descent = other.m_descent;
        m_lineHeight = other.m_lineHeight;
        m_screenWidth = other.m_screenWidth;
        m_screenHeight = other.m_screenHeight;
        m_edgeValue = other.m_edgeValue;
        m_smoothing = other.m_smoothing;
        m_uniformProjection = other.m_uniformProjection;
        m_uniformTextColor = other.m_uniformTextColor;
        m_uniformEdgeValue = other.m_uniformEdgeValue;
        m_uniformSmoothing = other.m_uniformSmoothing;
        m_projection = other.m_projection;
        m_fallbackGlyph = other.m_fallbackGlyph;

        other.m_texture = 0;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_shader = 0;
        other.m_atlasWidth = 0;
        other.m_atlasHeight = 0;
        other.m_scale = 1.0f;
        other.m_ascent = 0.0f;
        other.m_descent = 0.0f;
        other.m_lineHeight = 0.0f;
        other.m_screenWidth = 0;
        other.m_screenHeight = 0;
        other.m_edgeValue = 0.0f;
        other.m_smoothing = 0.0f;
        other.m_uniformProjection = -1;
        other.m_uniformTextColor = -1;
        other.m_uniformEdgeValue = -1;
        other.m_uniformSmoothing = -1;
        other.m_projection = {};
        other.m_fallbackGlyph = nullptr;
    }

    void TextRenderer::setScreenSize(int width, int height) {
        m_screenWidth = width;
        m_screenHeight = height;
        updateProjection();
    }

    void TextRenderer::renderText(const std::string& text, float x, float y, float scale, Color color) {
        if (text.empty() || scale <= 0.0f || m_shader == 0 || m_texture == 0) {
            return;
        }

        std::vector<float> vertices;
        vertices.reserve(text.size() * 6 * 4);

        float cursorX = x;
        float cursorY = y;

        glUseProgram(m_shader);
        glUniformMatrix4fv(m_uniformProjection, 1, GL_FALSE, m_projection.data());
        glUniform3f(m_uniformTextColor, color.r, color.g, color.b);
        glUniform1f(m_uniformSmoothing, m_smoothing / scale);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindVertexArray(m_vao);

        const size_t vertexStride = 4;

        for (size_t i = 0; i < text.size(); ++i) {
            const char c = text[i];
            if (c == '\n') {
                cursorX = x;
                cursorY -= m_lineHeight * scale;
                continue;
            }

            const Glyph* glyph = nullptr;
            if (auto it = m_glyphs.find(c); it != m_glyphs.end()) {
                glyph = &it->second;
            } else {
                glyph = m_fallbackGlyph;
            }

            if (!glyph) {
                continue;
            }

            float xPos = cursorX + glyph->xOffset * scale;
            float yPos = cursorY + glyph->yOffset * scale;
            float w = glyph->width * scale;
            float h = glyph->height * scale;

            if (w > 0.0f && h > 0.0f) {
                vertices.insert(vertices.end(), {
                    xPos,         yPos,         glyph->u0, glyph->v0,
                    xPos + w,     yPos,         glyph->u1, glyph->v0,
                    xPos + w,     yPos + h,     glyph->u1, glyph->v1,

                    xPos,         yPos,         glyph->u0, glyph->v0,
                    xPos + w,     yPos + h,     glyph->u1, glyph->v1,
                    xPos,         yPos + h,     glyph->u0, glyph->v1,
                });
            }

            cursorX += glyph->advance * scale;

            if (i + 1 < text.size() && m_fontInfo) {
                const char nextChar = text[i + 1];
                if (nextChar != '\n') {
                    int nextGlyphIndex = 0;
                    if (auto itNext = m_glyphs.find(nextChar); itNext != m_glyphs.end()) {
                        nextGlyphIndex = itNext->second.glyphIndex;
                    } else if (m_fallbackGlyph) {
                        nextGlyphIndex = m_fallbackGlyph->glyphIndex;
                    }
                    if (glyph->glyphIndex >= 0 && nextGlyphIndex >= 0) {
                        int kern = stbtt_GetGlyphKernAdvance(m_fontInfo.get(), glyph->glyphIndex, nextGlyphIndex);
                        cursorX += static_cast<float>(kern) * m_scale * scale;
                    }
                }
            }
        }

        if (!vertices.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(vertices.size() / vertexStride));
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    bool TextRenderer::loadFont(const std::string& fontPath) {
        const auto resolvedPath = resolveFontPath(fontPath);
        if (resolvedPath.empty()) {
            return false;
        }

        std::ifstream file(resolvedPath, std::ios::binary);
        if (!file) {
            return false;
        }

        file.seekg(0, std::ios::end);
        const std::streampos end = file.tellg();
        if (end <= 0) {
            return false;
        }
        file.seekg(0, std::ios::beg);

        const std::streamsize size = static_cast<std::streamsize>(end);
        m_fontBuffer.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(m_fontBuffer.data()), size);
        if (!file) {
            return false;
        }

        if (!stbtt_InitFont(m_fontInfo.get(), m_fontBuffer.data(), 0)) {
            return false;
        }

        return true;
    }

    void TextRenderer::buildAtlas(float pixelHeight) {
        if (!m_fontInfo) {
            throw std::runtime_error("Font data not loaded");
        }

        m_glyphs.clear();
        m_glyphs.reserve(static_cast<size_t>(kLastChar - kFirstChar + 1));

        m_atlasWidth = kAtlasSize;
        m_atlasHeight = kAtlasSize;
        m_atlasData.assign(static_cast<size_t>(m_atlasWidth * m_atlasHeight), 0);

        m_scale = stbtt_ScaleForPixelHeight(m_fontInfo.get(), pixelHeight);

        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(m_fontInfo.get(), &ascent, &descent, &lineGap);
        m_ascent = ascent * m_scale;
        m_descent = descent * m_scale;
        m_lineHeight = (ascent - descent + lineGap) * m_scale;

        int penX = kPadding;
        int penY = kPadding;
        int rowHeight = 0;

        for (int codepoint = kFirstChar; codepoint <= kLastChar; ++codepoint) {
            int glyphIndex = stbtt_FindGlyphIndex(m_fontInfo.get(), codepoint);

            int width = 0;
            int height = 0;
            int xoff = 0;
            int yoff = 0;
            unsigned char* sdf = stbtt_GetGlyphSDF(
                m_fontInfo.get(),
                m_scale,
                glyphIndex,
                kPadding,
                kOnEdgeValue,
                kPixelDistScale,
                &width,
                &height,
                &xoff,
                &yoff);

            if (!sdf) {
                throw std::runtime_error("Failed to generate SDF for glyph");
            }

            if (width > 0 && height > 0) {
                if (penX + width + kPadding > m_atlasWidth) {
                    penX = kPadding;
                    penY += rowHeight + kPadding;
                    rowHeight = 0;
                }

                if (penY + height + kPadding > m_atlasHeight) {
                    stbtt_FreeSDF(sdf, nullptr);
                    throw std::runtime_error("Font atlas too small for requested glyphs");
                }

                for (int row = 0; row < height; ++row) {
                    unsigned char* dest = m_atlasData.data() + (penY + row) * m_atlasWidth + penX;
                    unsigned char* src = sdf + row * width;
                    std::copy(src, src + width, dest);
                }

                rowHeight = std::max(rowHeight, height);
            }

            int advanceWidth = 0;
            int leftBearing = 0;
            stbtt_GetGlyphHMetrics(m_fontInfo.get(), glyphIndex, &advanceWidth, &leftBearing);

            Glyph glyph{};
            glyph.advance = advanceWidth * m_scale;
            glyph.xOffset = static_cast<float>(xoff);
            glyph.yOffset = static_cast<float>(yoff);
            glyph.width = static_cast<float>(width);
            glyph.height = static_cast<float>(height);
            glyph.u0 = width > 0 ? static_cast<float>(penX) / static_cast<float>(m_atlasWidth) : 0.0f;
            glyph.v0 = height > 0 ? static_cast<float>(penY) / static_cast<float>(m_atlasHeight) : 0.0f;
            glyph.u1 = width > 0 ? static_cast<float>(penX + width) / static_cast<float>(m_atlasWidth) : 0.0f;
            glyph.v1 = height > 0 ? static_cast<float>(penY + height) / static_cast<float>(m_atlasHeight) : 0.0f;
            glyph.glyphIndex = glyphIndex;

            m_glyphs.emplace(static_cast<char>(codepoint), glyph);

            if (width > 0) {
                penX += width + kPadding;
            } else {
                penX += static_cast<int>(glyph.advance) + kPadding;
            }

            stbtt_FreeSDF(sdf, nullptr);
        }

        if (auto it = m_glyphs.find('?'); it != m_glyphs.end()) {
            m_fallbackGlyph = &it->second;
        } else if (!m_glyphs.empty()) {
            m_fallbackGlyph = &m_glyphs.begin()->second;
        } else {
            m_fallbackGlyph = nullptr;
        }

        m_edgeValue = static_cast<float>(kOnEdgeValue) / 255.0f;
        m_smoothing = 0.5f / kPixelDistScale;
    }

    void TextRenderer::createShader() {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSrc);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
        m_shader = linkProgram(vertexShader, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glUseProgram(m_shader);
        GLint textureLocation = glGetUniformLocation(m_shader, "uTexture");
        glUniform1i(textureLocation, 0);
        m_uniformProjection = glGetUniformLocation(m_shader, "uProjection");
        m_uniformTextColor = glGetUniformLocation(m_shader, "uTextColor");
        m_uniformEdgeValue = glGetUniformLocation(m_shader, "uEdgeValue");
        m_uniformSmoothing = glGetUniformLocation(m_shader, "uSmoothing");
        glUniform1f(m_uniformEdgeValue, m_edgeValue);
        glUniform1f(m_uniformSmoothing, m_smoothing);
        glUseProgram(0);
    }

    void TextRenderer::createBuffers() {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * 6 * 4), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void TextRenderer::uploadAtlasTexture() {
        if (m_atlasData.empty()) {
            throw std::runtime_error("Atlas data is empty");
        }

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_atlasWidth, m_atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, m_atlasData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void TextRenderer::updateProjection() {
        const float left = 0.0f;
        const float right = static_cast<float>(m_screenWidth);
        const float bottom = 0.0f;
        const float top = static_cast<float>(m_screenHeight);
        const float nearPlane = -1.0f;
        const float farPlane = 1.0f;

        const float rl = right - left;
        const float tb = top - bottom;
        const float fn = farPlane - nearPlane;

        if (rl == 0.0f || tb == 0.0f || fn == 0.0f) {
            m_projection = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
        } else {
            m_projection = {
                2.0f / rl, 0.0f,       0.0f,        0.0f,
                0.0f,      2.0f / tb,  0.0f,        0.0f,
                0.0f,      0.0f,      -2.0f / fn,   0.0f,
                -(right + left) / rl,
                -(top + bottom) / tb,
                -(farPlane + nearPlane) / fn,
                1.0f
            };
        }

        if (m_shader != 0) {
            glUseProgram(m_shader);
            glUniformMatrix4fv(m_uniformProjection, 1, GL_FALSE, m_projection.data());
            glUseProgram(0);
        }
    }

} // namespace WidgeCraft
