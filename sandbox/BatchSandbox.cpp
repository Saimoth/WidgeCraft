#include "WidgeCraft/WidgeCraft.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {

    class BatchDemo {
    public:
        explicit BatchDemo(WidgeCraft::WidgeCraft& app) {
            const std::filesystem::path modelDirectory =
                std::filesystem::path(WIDGECRAFT_ASSET_DIR) / "models";
            m_pyramid = std::make_shared<WidgeCraft::Mesh3D>(
                WidgeCraft::ModelLoader::load(
                    modelDirectory / "pyramid.obj"));
            m_tetrahedron = std::make_shared<WidgeCraft::Mesh3D>(
                WidgeCraft::ModelLoader::load(
                    modelDirectory / "tetrahedron.stl"));

            constexpr int columns = 40;
            constexpr int rows = 25;
            constexpr float spacing = 4.0f;
            for (int row = 0; row < rows; ++row) {
                for (int column = 0; column < columns; ++column) {
                    const int index = row * columns + column;
                    WidgeCraft::Transform3D transform;
                    transform.position = {
                        (static_cast<float>(column)
                            - static_cast<float>(columns - 1) * 0.5f)
                            * spacing,
                        0.9f + 0.45f * std::sin(
                            static_cast<float>(index) * 0.17f),
                        (static_cast<float>(row)
                            - static_cast<float>(rows - 1) * 0.5f)
                            * spacing
                    };
                    transform.rotation.y = static_cast<float>(index) * 0.13f;
                    transform.scale = { 0.8f, 0.8f, 0.8f };

                    auto& object = m_objects.createMeshObject(
                        "Stress object " + std::to_string(index + 1),
                        index % 2 == 0 ? m_pyramid : m_tetrahedron,
                        transform);
                    object.getAttributes().labelVisible = true;
                    object.getAttributes().interactable = false;
                    object.getStyle().edgeVisible = false;
                    object.getStyle().fillColor = index % 2 == 0
                        ? WidgeCraft::Color{ 0.15f, 0.50f, 0.84f, 1.0f }
                        : WidgeCraft::Color{ 0.82f, 0.34f, 0.16f, 1.0f };
                }
            }
            m_objects.setDrawDistance(m_drawDistance);

            auto& camera = m_viewport.getCamera3D();
            camera.setPerspective(55.0f, 0.1f, m_drawDistance);
            updateCamera();

            auto& panel = app.getRootFrame().createChildFrame("Batch HUD");
            panel.setAnchor(WidgeCraft::Anchor::TopLeft);
            panel.setPosition(20.0f, 20.0f);
            panel.setSize(620.0f, 214.0f);
            panel.setBackgroundColor({ 0.025f, 0.038f, 0.060f, 0.94f });
            panel.setBorderVisible(true);
            panel.setBorderColor({ 0.30f, 0.58f, 0.82f, 1.0f });
            panel.setBorderThickness(2.0f);

            auto& title = panel.addWidget<WidgeCraft::Label>(
                "Title",
                "1,000-object batch and label stress test");
            title.setAnchor(WidgeCraft::Anchor::TopLeft);
            title.setPosition(16.0f, 12.0f);
            title.setTextSize(23.0f);
            title.setColor({ 0.76f, 0.92f, 1.0f, 1.0f });

            m_distanceLabel = &panel.addWidget<WidgeCraft::Label>(
                "Distance",
                "Draw distance: 55");
            m_distanceLabel->setAnchor(WidgeCraft::Anchor::TopLeft);
            m_distanceLabel->setPosition(16.0f, 50.0f);
            m_distanceLabel->setTextSize(16.0f);

            auto& slider = panel.addWidget<WidgeCraft::Slider>(
                "Draw Distance",
                12.0f,
                180.0f,
                m_drawDistance);
            slider.setAnchor(WidgeCraft::Anchor::TopLeft);
            slider.setPosition(16.0f, 78.0f);
            slider.setSize(580.0f, 32.0f);
            slider.setStep(1.0f);
            slider.setOnChanged([this](float distance) {
                m_drawDistance = distance;
                m_objects.setDrawDistance(distance);
                m_viewport.setDrawDistance(distance);
                m_distanceLabel->setText(
                    "Draw distance: "
                    + std::to_string(static_cast<int>(distance)));
            });

            m_statsLabel = &panel.addWidget<WidgeCraft::Label>(
                "Stats",
                "Collecting batch statistics...");
            m_statsLabel->setAnchor(WidgeCraft::Anchor::BottomLeft);
            m_statsLabel->setPosition(16.0f, 48.0f);
            m_statsLabel->setTextSize(15.0f);
            m_statsLabel->setColor({ 0.64f, 0.88f, 0.72f, 1.0f });

            m_memoryLabel = &panel.addWidget<WidgeCraft::Label>(
                "Memory",
                "Scene memory");
            m_memoryLabel->setAnchor(WidgeCraft::Anchor::BottomLeft);
            m_memoryLabel->setPosition(16.0f, 18.0f);
            m_memoryLabel->setTextSize(15.0f);
            m_memoryLabel->setColor({ 0.68f, 0.78f, 0.90f, 1.0f });
        }

        void update(WidgeCraft::WidgeCraft& app) {
            setViewport(app);
            const auto& input = app.getInput();
            if (input.mouseDown(WidgeCraft::MouseButton::Right)) {
                const WidgeCraft::Vec2 delta = input.mouseDelta();
                m_yaw += delta.x * 0.005f;
                m_pitch = std::clamp(
                    m_pitch + delta.y * 0.005f,
                    -0.95f,
                    0.65f);
            }

            const WidgeCraft::Vec3 forward{
                std::sin(m_yaw),
                0.0f,
                -std::cos(m_yaw)
            };
            const WidgeCraft::Vec3 right{
                std::cos(m_yaw),
                0.0f,
                std::sin(m_yaw)
            };
            WidgeCraft::Vec3 movement{};
            if (input.keyDown(WidgeCraft::Key::W)) {
                movement += forward;
            }
            if (input.keyDown(WidgeCraft::Key::S)) {
                movement -= forward;
            }
            if (input.keyDown(WidgeCraft::Key::D)) {
                movement += right;
            }
            if (input.keyDown(WidgeCraft::Key::A)) {
                movement -= right;
            }
            if (WidgeCraft::lengthSquared(movement) > 1.0f) {
                movement = WidgeCraft::normalized(movement);
            }
            m_cameraTarget += movement
                * (24.0f * app.getDeltaTime());
            updateCamera();

            std::ostringstream stats;
            stats << std::fixed << std::setprecision(3)
                  << "Visible " << m_stats.rendered << " / 1000"
                  << "  |  culled " << m_stats.distanceCulled
                  << "  |  labels " << m_stats.labelsQueued
                  << "  |  CPU queue " << m_stats.cpuMilliseconds << " ms";
            m_statsLabel->setText(stats.str());

            const double sceneMegabytes = static_cast<double>(
                m_objects.getApproximateMemoryUsageBytes())
                / (1024.0 * 1024.0);
            const double queueKilobytes = static_cast<double>(
                m_stats.vertexBytesQueued)
                / 1024.0;
            std::ostringstream memory;
            memory << std::fixed << std::setprecision(2)
                   << "Triangles " << m_stats.trianglesQueued
                   << "  |  vertex queue " << queueKilobytes << " KiB"
                   << "  |  scene " << sceneMegabytes << " MiB"
                   << "  |  1 mesh draw + 1 SDF text draw";
            m_memoryLabel->setText(memory.str());
        }

        void render(WidgeCraft::WidgeCraft& app) {
            setViewport(app);
            updateCamera();

            WidgeCraft::ViewportRenderOptions options;
            options.clearColor = true;
            options.color = { 0.018f, 0.026f, 0.042f, 1.0f };
            app.renderViewport(
                m_viewport,
                [this](WidgeCraft::WidgeCraft& engine) {
                    m_stats = m_objects.render(
                        engine.getShapes3D(),
                        m_viewport.getCamera3D().getPosition(),
                        &m_viewport,
                        &engine.getTextRenderer());
                },
                options);
        }

    private:
        void setViewport(WidgeCraft::WidgeCraft& app) {
            m_viewport.setScreenRect({
                0.0f,
                0.0f,
                static_cast<float>(app.getWindow().getWidth()),
                static_cast<float>(app.getWindow().getHeight())
            });
        }

        void updateCamera() {
            const float horizontal = std::cos(m_pitch) * 24.0f;
            const WidgeCraft::Vec3 offset{
                horizontal * std::sin(m_yaw),
                11.0f + std::sin(m_pitch) * 24.0f,
                horizontal * std::cos(m_yaw)
            };
            auto& camera = m_viewport.getCamera3D();
            camera.setPosition(m_cameraTarget + offset);
            camera.setTarget(m_cameraTarget);
        }

        std::shared_ptr<WidgeCraft::Mesh3D> m_pyramid;
        std::shared_ptr<WidgeCraft::Mesh3D> m_tetrahedron;
        WidgeCraft::ObjectManager m_objects;
        WidgeCraft::ObjectRenderStats m_stats;
        WidgeCraft::ModelViewport m_viewport;
        WidgeCraft::Label* m_distanceLabel = nullptr;
        WidgeCraft::Label* m_statsLabel = nullptr;
        WidgeCraft::Label* m_memoryLabel = nullptr;
        WidgeCraft::Vec3 m_cameraTarget{};
        float m_drawDistance = 55.0f;
        float m_yaw = 0.0f;
        float m_pitch = -0.22f;
    };

} // namespace

int main() {
    try {
        WidgeCraft::WidgeCraft app(
            "WidgeCraft Batch and Draw Distance Sandbox",
            1280,
            800);
        BatchDemo demo(app);
        app.setUpdateCallback(
            [&demo](WidgeCraft::WidgeCraft& engine) {
                demo.update(engine);
            });
        app.setRenderCallback(
            [&demo](WidgeCraft::WidgeCraft& engine) {
                demo.render(engine);
            });
        app.Run(60);
    } catch (const std::exception& exception) {
        std::cerr << "WidgeCraft batch sandbox failed: "
                  << exception.what() << '\n';
        return 1;
    }
    return 0;
}
