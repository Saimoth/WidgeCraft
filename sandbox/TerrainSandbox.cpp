#include "WidgeCraft/WidgeCraft.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {

    class TerrainDemo {
    public:
        explicit TerrainDemo(WidgeCraft::WidgeCraft& app)
            : m_heightMap(std::make_shared<WidgeCraft::HeightMap>(
                WidgeCraft::HeightMap::generateGradual(
                    65,
                    65,
                    1.0f,
                    0.58f,
                    1337,
                    3,
                    { -32.0f, 0.0f, -32.0f })))
            , m_terrain(m_heightMap) {

            m_physics.setGravity({ 0.0f, -20.0f, 0.0f });
            m_physics.setHeightMapCollider(m_heightMap);
            const float startHeight = m_heightMap->sampleHeight(0.0f, 0.0f)
                .value_or(0.0f);
            m_player = &m_physics.createBody(
                WidgeCraft::PhysicsBodyType::Dynamic,
                { 0.0f, startHeight + PlayerHalfHeight, 0.0f },
                WidgeCraft::BoxCollider({
                    PlayerHalfWidth,
                    PlayerHalfHeight,
                    PlayerHalfWidth
                }));
            m_player->setMass(1.0f);

            auto& camera = m_viewport.getCamera3D();
            camera.setPerspective(55.0f, 0.1f, DrawDistance);
            updateCamera();

            auto& panel = app.getRootFrame().createChildFrame("Terrain HUD");
            panel.setAnchor(WidgeCraft::Anchor::TopLeft);
            panel.setPosition(20.0f, 20.0f);
            panel.setSize(560.0f, 136.0f);
            panel.setBackgroundColor({ 0.025f, 0.045f, 0.055f, 0.92f });
            panel.setBorderVisible(true);
            panel.setBorderColor({ 0.30f, 0.68f, 0.54f, 1.0f });
            panel.setBorderThickness(2.0f);

            auto& title = panel.addWidget<WidgeCraft::Label>(
                "Title",
                "Gradual heightmap terrain");
            title.setAnchor(WidgeCraft::Anchor::TopLeft);
            title.setPosition(16.0f, 12.0f);
            title.setTextSize(23.0f);
            title.setColor({ 0.72f, 1.0f, 0.82f, 1.0f });

            auto& help = panel.addWidget<WidgeCraft::Label>(
                "Help",
                "WASD move  |  Space jump  |  Right-drag look  |  Wheel zoom");
            help.setAnchor(WidgeCraft::Anchor::TopLeft);
            help.setPosition(16.0f, 48.0f);
            help.setTextSize(15.0f);
            help.setColor({ 0.72f, 0.82f, 0.86f, 1.0f });

            m_status = &panel.addWidget<WidgeCraft::Label>(
                "Status",
                "Initialising terrain physics...");
            m_status->setAnchor(WidgeCraft::Anchor::BottomLeft);
            m_status->setPosition(16.0f, 18.0f);
            m_status->setTextSize(16.0f);
            m_status->setColor({ 0.58f, 0.92f, 0.68f, 1.0f });
        }

        void update(WidgeCraft::WidgeCraft& app) {
            setViewport(app);
            const float deltaTime = app.getDeltaTime();
            const auto& input = app.getInput();

            m_cameraDistance = std::clamp(
                m_cameraDistance - input.scrollDelta().y * 1.1f,
                2.5f,
                14.0f);
            if (input.mouseDown(WidgeCraft::MouseButton::Right)) {
                const WidgeCraft::Vec2 delta = input.mouseDelta();
                m_yaw += delta.x * 0.006f;
                m_pitch = std::clamp(
                    m_pitch + delta.y * 0.006f,
                    -1.0f,
                    0.55f);
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

            WidgeCraft::Vec3 velocity = m_player->getVelocity();
            velocity.x = movement.x * MovementSpeed;
            velocity.z = movement.z * MovementSpeed;
            if (input.keyPressed(WidgeCraft::Key::Space)
                && m_player->isGrounded()) {
                velocity.y = JumpVelocity;
            }
            m_player->setVelocity(velocity);
            m_physics.step(deltaTime);

            if (m_player->getPosition().y < -20.0f) {
                const float resetHeight = m_heightMap->sampleHeight(0.0f, 0.0f)
                    .value_or(0.0f);
                m_player->setPosition({
                    0.0f,
                    resetHeight + PlayerHalfHeight + 0.1f,
                    0.0f
                });
                m_player->setVelocity({});
            }

            updateCamera();
            std::ostringstream status;
            status << std::fixed << std::setprecision(2)
                   << "Position " << m_player->getPosition().x
                   << ", " << m_player->getPosition().y
                   << ", " << m_player->getPosition().z
                   << "  |  "
                   << (m_player->isGrounded() ? "Grounded" : "Airborne")
                   << "  |  8,192 batched terrain triangles";
            m_status->setText(status.str());
        }

        void render(WidgeCraft::WidgeCraft& app) {
            setViewport(app);
            updateCamera();

            WidgeCraft::ViewportRenderOptions options;
            options.clearColor = true;
            options.color = { 0.035f, 0.065f, 0.080f, 1.0f };
            app.renderViewport(
                m_viewport,
                [this](WidgeCraft::WidgeCraft& engine) {
                    WidgeCraft::ShapeStyle3D terrainStyle;
                    terrainStyle.fillColor = {
                        0.18f,
                        0.48f,
                        0.25f,
                        1.0f
                    };
                    terrainStyle.edgeVisible = false;
                    m_terrain.draw(
                        engine.getShapes3D(),
                        terrainStyle,
                        m_player->getPosition(),
                        DrawDistance);

                    WidgeCraft::ShapeStyle3D playerStyle;
                    playerStyle.fillColor = {
                        0.12f,
                        0.52f,
                        0.88f,
                        1.0f
                    };
                    playerStyle.edgeColor = {
                        0.76f,
                        0.94f,
                        1.0f,
                        1.0f
                    };
                    playerStyle.edgeThickness = 2.0f;
                    const WidgeCraft::AABB bounds = m_player->getBounds();
                    engine.getShapes3D().drawBox(
                        bounds.minimum,
                        bounds.maximum,
                        playerStyle);
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

        WidgeCraft::Vec3 viewForward() const {
            const float pitchScale = std::cos(m_pitch);
            return WidgeCraft::normalized({
                pitchScale * std::sin(m_yaw),
                std::sin(m_pitch),
                -pitchScale * std::cos(m_yaw)
            });
        }

        void updateCamera() {
            const WidgeCraft::Vec3 eye = m_player->getPosition()
                + WidgeCraft::Vec3{ 0.0f, 0.62f, 0.0f };
            const WidgeCraft::Vec3 forward = viewForward();
            WidgeCraft::Vec3 cameraPosition = eye
                - forward * m_cameraDistance;
            const auto terrainHeight = m_heightMap->sampleHeight(
                cameraPosition.x,
                cameraPosition.z);
            if (terrainHeight) {
                cameraPosition.y = std::max(
                    cameraPosition.y,
                    *terrainHeight + 0.4f);
            }
            auto& camera = m_viewport.getCamera3D();
            camera.setPosition(cameraPosition);
            camera.setTarget(eye);
        }

        static constexpr float PlayerHalfWidth = 0.42f;
        static constexpr float PlayerHalfHeight = 0.85f;
        static constexpr float MovementSpeed = 6.0f;
        static constexpr float JumpVelocity = 8.5f;
        static constexpr float DrawDistance = 72.0f;

        std::shared_ptr<WidgeCraft::HeightMap> m_heightMap;
        WidgeCraft::Terrain m_terrain;
        WidgeCraft::PhysicsWorld m_physics;
        WidgeCraft::PhysicsBody* m_player = nullptr;
        WidgeCraft::ModelViewport m_viewport;
        WidgeCraft::Label* m_status = nullptr;
        float m_yaw = 0.0f;
        float m_pitch = -0.34f;
        float m_cameraDistance = 10.0f;
    };

} // namespace

int main() {
    try {
        WidgeCraft::WidgeCraft app(
            "WidgeCraft Heightmap Terrain Sandbox",
            1200,
            760);
        TerrainDemo demo(app);
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
        std::cerr << "WidgeCraft terrain sandbox failed: "
                  << exception.what() << '\n';
        return 1;
    }
    return 0;
}
