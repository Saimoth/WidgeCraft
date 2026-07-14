#include "WidgeCraft/WidgeCraft.hpp"

// GLAD must be included before GLFW.
#include <glad/glad.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <stdexcept>
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
        , m_shapes3D()
        , m_sceneManager(*this)
        , m_uiManager(*this) {
        initializeGraphics();
    }

    WidgeCraft::~WidgeCraft() {
        m_uiManager.shutdown();
        m_sceneManager.shutdown();
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
        m_sceneManager.setTransient(std::move(scene));
    }

    void WidgeCraft::useModelViewport(
        const ModelViewport& viewport) {

        // A renderer stores only one current 3D matrix. Flush anything already
        // queued before selecting the next camera/clip combination.
        flushSceneQueues();

        m_modelViewportRect = viewport.getScreenRect();
        m_modelViewportActive =
            m_modelViewportRect.width > 0.0f
            && m_modelViewportRect.height > 0.0f;
        m_modelViewportOptions = {};

        if (!m_modelViewportActive) {
            m_shapes2D.resetTransform();
            return;
        }

        viewport.configureRenderers(m_shapes2D, m_shapes3D);
    }

    void WidgeCraft::clearModelViewport() {
        if (m_modelViewportActive) {
            flushSceneQueues();
        } else {
            m_shapes2D.resetTransform();
        }
    }

    void WidgeCraft::renderViewport(
        const ModelViewport& viewport,
        const ViewportRenderCallback& callback,
        const ViewportRenderOptions& options) {

        if (!callback) {
            return;
        }
        if (m_viewportCallbackActive) {
            throw std::logic_error(
                "Viewport render callbacks cannot be nested");
        }

        flushSceneQueues();
        m_modelViewportRect = viewport.getScreenRect();
        m_modelViewportActive =
            m_modelViewportRect.width > 0.0f
            && m_modelViewportRect.height > 0.0f;
        m_modelViewportOptions = options;

        if (!m_modelViewportActive) {
            return;
        }

        viewport.configureRenderers(m_shapes2D, m_shapes3D);
        m_viewportCallbackActive = true;
        try {
            callback(*this);
            m_viewportCallbackActive = false;
            flushSceneQueues();
        } catch (...) {
            m_viewportCallbackActive = false;
            m_shapes3D.beginFrame();
            m_shapes2D.beginFrame();
            m_textRenderer.beginFrame();
            m_modelViewportActive = false;
            m_modelViewportRect = {};
            restoreFullFramebufferState();
            throw;
        }
    }

    void WidgeCraft::Update() {
        m_textRenderer.setScreenSize(
            m_window.getWidth(),
            m_window.getHeight());
        m_shapes2D.setScreenSize(
            static_cast<float>(m_window.getWidth()),
            static_cast<float>(m_window.getHeight()));

        // Scene and UI requests are applied together at the same safe frame
        // boundary before either newly active object receives an update.
        m_sceneManager.applyPending();
        m_uiManager.applyPending();
        m_sceneManager.update(m_deltaTime);
        m_uiManager.update(m_deltaTime);
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
        m_modelViewportOptions = {};

        m_sceneManager.render();
        if (m_renderCallback) {
            m_renderCallback(*this);
        }
        m_uiManager.renderSceneViews();

        flushSceneQueues();

        // Retained UI always renders in native framebuffer pixels. Scene
        // transforms, clipping and the 3D viewport cannot stretch or crop it.
        restoreFullFramebufferState();
        m_window.getRootFrame().render(
            m_textRenderer,
            m_shapes2D,
            m_window.getInput());
        m_uiManager.renderUi();
        m_shapes2D.flush();
        m_textRenderer.flush();
    }

    void WidgeCraft::flushSceneQueues() {
        const int framebufferWidth = m_window.getWidth();
        const int framebufferHeight = m_window.getHeight();

        if (!m_modelViewportActive) {
            restoreFullFramebufferState();
            m_shapes3D.flush();
            m_shapes2D.flush();
            m_textRenderer.flush();
            return;
        }

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
        glViewport(left, bottom, clipWidth, clipHeight);

        GLbitfield clearMask = 0;
        if (m_modelViewportOptions.clearColor) {
            const Color color = m_modelViewportOptions.color;
            glClearColor(color.r, color.g, color.b, color.a);
            clearMask |= GL_COLOR_BUFFER_BIT;
        }
        if (m_modelViewportOptions.clearDepth) {
            clearMask |= GL_DEPTH_BUFFER_BIT;
        }
        if (clearMask != 0 && clipWidth > 0 && clipHeight > 0) {
            glClear(clearMask);
        }

        // 3D uses the model rectangle as its OpenGL viewport. Its depth area is
        // cleared per pass so an overlaid minimap cannot inherit world depth.
        m_shapes3D.flush();

        // 2D vertices and scene text use full-framebuffer pixel coordinates;
        // the viewport's scissor remains active while the full viewport returns.
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        m_shapes2D.flush();
        m_textRenderer.flush();

        m_modelViewportActive = false;
        m_modelViewportRect = {};
        m_modelViewportOptions = {};
        restoreFullFramebufferState();
    }

    void WidgeCraft::restoreFullFramebufferState() {
        glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
        glDisable(GL_SCISSOR_TEST);
        glClearColor(
            m_clearColor.r,
            m_clearColor.g,
            m_clearColor.b,
            m_clearColor.a);
        m_shapes2D.resetTransform();
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
