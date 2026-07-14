#include "WidgeCraft/WidgeCraft.hpp"

// GLAD must be included before GLFW.
#include <glad/glad.h>

#include <algorithm>
#include <chrono>
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
        glClearColor(
            m_clearColor.r,
            m_clearColor.g,
            m_clearColor.b,
            m_clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shapes3D.beginFrame();
        m_shapes2D.beginFrame();
        m_textRenderer.beginFrame();

        if (m_scene) {
            m_scene->onRender(*this);
        }
        if (m_renderCallback) {
            m_renderCallback(*this);
        }

        // World geometry first, then the transformed 2D scene.
        m_shapes3D.flush();
        m_shapes2D.flush();
        m_textRenderer.flush();

        // Retained UI always uses native client pixels, regardless of any
        // logical scene transform used by the callback or Scene.
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
