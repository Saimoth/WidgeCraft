#include "WidgeCraft/WidgeCraft.hpp"

#include "WidgeCraft/ShapeRenderer.hpp"
#include "WidgeCraft/TextRenderer.hpp"

// Order matters: glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <utility>

namespace WidgeCraft {

    namespace {
        std::string resolveDefaultFontPath() {
            const std::filesystem::path defaultPath = std::filesystem::path(WIDGECRAFT_ASSET_DIR) / "fonts/Roboto-Regular.ttf";
            return defaultPath.string();
        }
    }

    WidgeCraft::WidgeCraft(std::string title, int width, int height, std::string fontPath, float fontPixelHeight)
        : m_window(width, height, std::move(title))
        , m_textRenderer(m_window.getWidth(), m_window.getHeight(), resolveFontPath(fontPath), fontPixelHeight)
        , m_shapeRenderer() {
        initializeGraphics();
    }

    void WidgeCraft::Run() {
        while (!m_window.shouldClose()) {
            Update();
            Render();

            glfwSwapBuffers(m_window.getNativeHandle());
            m_window.pollEvents();
        }
    }

    void WidgeCraft::Update() {
        m_textRenderer.setScreenSize(m_window.getWidth(), m_window.getHeight());
        m_shapeRenderer.setScreenSize(static_cast<float>(m_window.getWidth()), static_cast<float>(m_window.getHeight()));

        if (m_updateCallback) {
            m_updateCallback(*this);
        }
    }

    void WidgeCraft::Render() {
        glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_renderCallback) {
            m_renderCallback(*this);
        }

        m_window.getRootFrame().render(m_textRenderer, m_shapeRenderer);
    }

    void WidgeCraft::initializeGraphics() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    std::string WidgeCraft::resolveFontPath(const std::string& fontPath) {
        if (!fontPath.empty()) {
            return fontPath;
        }

        return resolveDefaultFontPath();
    }

} // namespace WidgeCraft

