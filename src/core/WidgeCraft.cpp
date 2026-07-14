#include "WidgeCraft/WidgeCraft.hpp"

// GLAD must be included before GLFW.
#include <glad/glad.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <thread>
#include <utility>

namespace WidgeCraft {

    namespace {
        std::string resolveDefaultFontPath() {
            const std::filesystem::path configuredPath =
                std::filesystem::path(WIDGECRAFT_ASSET_DIR)
                / "fonts/Roboto-Regular.ttf";
            if (std::filesystem::exists(configuredPath)) {
                return configuredPath.string();
            }

            const std::filesystem::path localPath =
                std::filesystem::current_path()
                / "assets/fonts/Roboto-Regular.ttf";
            return localPath.string();
        }

        Rect clipToFramebuffer(
            const Rect& rect,
            int framebufferWidth,
            int framebufferHeight) {

            return intersect(
                rect,
                Rect{
                    0.0f,
                    0.0f,
                    static_cast<float>(std::max(framebufferWidth, 0)),
                    static_cast<float>(std::max(framebufferHeight, 0))
                });
        }
    } // namespace

    WidgeCraft::WidgeCraft(
        std::string title,
        int width,
        int height,
        std::string fontPath,
        float fontPixelHeight)
        : m_window(width, height, std::move(title))
        , m_textRenderer(
            m_window.getWidth(),
            m_window.getHeight(),
            resolveFontPath(fontPath),
            fontPixelHeight)
        , m_shapes2D()
        , m_shapes3D() {
        initializeGraphics();
    }

    WidgeCraft::~WidgeCraft() {
        if (m_scene) {
            m_scene->onDetach(*this);
        }
    }

    void WidgeCraft::Run(int targetFramesPerSecond) {
        using Clock = std::chrono::steady_clock;
        using Seconds = std::chrono::duration<double>;

        m_targetFrameRate = std::max(0, targetFramesPerSecond);
        auto previousFrame = Clock::now();

        while (!m_window.shouldClose()) {
            const auto frameStart = Clock::now();
            m_deltaTime = static_cast<float>(std::min(
                0.25,
                Seconds(frameStart - previousFrame).count()));
            previousFrame = frameStart;
            m_elapsedTime += static_cast<double>(m_deltaTime);

            m_window.pollEvents();
            Update();
            Render();
            m_window.swapBuffers();

            if (m_targetFrameRate > 0 && !m_window.isVSyncEnabled()) {
                const Seconds targetDuration(
                    1.0 / static_cast<double>(m_targetFrameRate));
                while (Clock::now() - frameStart < targetDuration) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1));
                }
            }
        }
    }

    void WidgeCraft::Stop() {
        m_window.requestClose();
    }

    void WidgeCraft::setScene(std::unique_ptr<Scene> scene) {
        if (m_scene) {
            m_scene->onDetach(*this);
        }
        m_scene = std::move(scene);
        if (m_scene) {
            m_scene->onAttach(*this);
        }
    }

    void WidgeCraft::useModelViewport(
        const ModelViewport& viewport) {

        m_modelViewportRect = viewport.getScreenRect();
        m_modelViewportActive =
            m_modelViewportRect.width > 0.0f
            && m_modelViewportRect.height > 0.0f;

        if (!m_modelViewportActive) {
            m_shapes2D.resetTransform();
            return;
        }

        viewport.configureRenderers(m_shapes2D, m_shapes3D);
    }

    void WidgeCraft::clearModelViewport() {
        m_modelViewportActive = false;
        m_modelViewportRect = {};
        m_shapes2D.resetTransform();
    }

    void WidgeCraft::Update() {
        m_textRenderer.setScreenSize(
            m_window.getWidth(),
            m_window.getHeight());
        m_shapes2D.setScreenSize(
            static_cast<float>(m_window.getWidth()),
            static_cast<float>(m_window.getHeight()));

        if (m_scene) {
            m_scene->onUpdate(*this, m_deltaTime);
        }
        if (m_updateCallback) {
            m_updateCallback(*this);
        }
    }

    void WidgeCraft::Render() {
        const int framebufferWidth = m_window.getWidth();
        const int framebufferHeight = m_window.getHeight();

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(
            m_clearColor.r,
            m_clearColor.g,
            m_clearColor.b,
            m_clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shapes3D.beginFrame();
        m_shapes2D.beginFrame();
        m_textRenderer.beginFrame();
        m_modelViewportActive = false;
        m_modelViewportRect = {};

        if (m_scene) {
            m_scene->onRender(*this);
        }
        if (m_renderCallback) {
            m_renderCallback(*this);
        }

        if (m_modelViewportActive) {
            const Rect clipped = clipToFramebuffer(
                m_modelViewportRect,
                framebufferWidth,
                framebufferHeight);

            const int left = static_cast<int>(std::floor(clipped.x));
            const int bottom = static_cast<int>(std::floor(clipped.y));
            const int right = static_cast<int>(std::ceil(clipped.right()));
            const int top = static_cast<int>(std::ceil(clipped.top()));
            const int clipWidth = std::max(0, right - left);
            const int clipHeight = std::max(0, top - bottom);

            glEnable(GL_SCISSOR_TEST);
            glScissor(left, bottom, clipWidth, clipHeight);

            // 3D uses the model rectangle as its OpenGL viewport. The camera
            // projection was built from the same aspect ratio.
            glViewport(left, bottom, clipWidth, clipHeight);
            m_shapes3D.flush();

            // 2D shapes and text are already transformed into full-framebuffer
            // coordinates, so only scissoring remains active for these passes.
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            m_shapes2D.flush();
            m_textRenderer.flush();

            glDisable(GL_SCISSOR_TEST);
        } else {
            m_shapes3D.flush();
            m_shapes2D.flush();
            m_textRenderer.flush();
        }

        // Retained UI always renders in native framebuffer pixels. Scene
        // transforms, clipping and the 3D viewport cannot stretch or crop it.
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glDisable(GL_SCISSOR_TEST);
        m_shapes2D.resetTransform();
        m_window.getRootFrame().render(
            m_textRenderer,
            m_shapes2D,
            m_window.getInput());
        m_shapes2D.flush();
        m_textRenderer.flush();
    }

    void WidgeCraft::initializeGraphics() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
    }

    std::string WidgeCraft::resolveFontPath(
        const std::string& fontPath) {
        return fontPath.empty() ? resolveDefaultFontPath() : fontPath;
    }

} // namespace WidgeCraft
