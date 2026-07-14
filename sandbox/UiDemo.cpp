#include "WidgeCraft/WidgeCraft.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

    constexpr const char* CharacterSceneName = "character-select";
    constexpr const char* WorldSceneName = "world";
    constexpr const char* CharacterUiName = "character-select";
    constexpr const char* CombatUiName = "combat";

    struct DemoState {
        int character = 0;
        WidgeCraft::Vec3 playerPosition{ 0.0f, 0.9f, 5.0f };
        WidgeCraft::Vec2 playerMapPosition{};
        float playerYaw = 0.0f;
        float cameraPitch = -0.24f;
        float cameraDistance = 8.0f;
        WidgeCraft::PhysicsBodyId selectedEnemyId = 0;
        bool playerGrounded = false;
    };

    struct EnemySpawn {
        WidgeCraft::PhysicsBodyId id = 0;
        WidgeCraft::Vec3 position{};
        WidgeCraft::Vec3 halfExtents{};
    };

    const std::array<EnemySpawn, 3> EnemySpawns{
        EnemySpawn{
            1001,
            { -3.2f, 0.9f, -2.5f },
            { 0.9f, 0.9f, 0.9f }
        },
        EnemySpawn{
            1002,
            { 4.0f, 0.65f, 2.0f },
            { 0.65f, 0.65f, 0.65f }
        },
        EnemySpawn{
            1003,
            { 1.0f, 1.1f, -7.0f },
            { 1.1f, 1.1f, 1.1f }
        }
    };

    void drawSelectionBackdrop(
        WidgeCraft::WidgeCraft& app,
        const DemoState& state) {

        auto& shapes = app.getShapes2D();
        auto& text = app.getTextRenderer();

        for (int x = -900; x <= 900; x += 60) {
            shapes.drawLine(
                static_cast<float>(x),
                -600.0f,
                static_cast<float>(x),
                600.0f,
                1.0f,
                { 0.08f, 0.13f, 0.20f, 0.55f });
        }
        for (int y = -600; y <= 600; y += 60) {
            shapes.drawLine(
                -900.0f,
                static_cast<float>(y),
                900.0f,
                static_cast<float>(y),
                1.0f,
                { 0.08f, 0.13f, 0.20f, 0.55f });
        }

        WidgeCraft::ShapeStyle2D portrait;
        portrait.fillColor = state.character == 0
            ? WidgeCraft::Color{ 0.10f, 0.34f, 0.58f, 0.92f }
            : WidgeCraft::Color{ 0.13f, 0.48f, 0.30f, 0.92f };
        portrait.edgeColor = state.character == 0
            ? WidgeCraft::Color{ 0.48f, 0.82f, 1.0f, 1.0f }
            : WidgeCraft::Color{ 0.48f, 1.0f, 0.68f, 1.0f };
        portrait.edgeThickness = 5.0f;
        portrait.edgeVisible = true;

        const float portraitX = state.character == 0 ? -360.0f : 360.0f;
        shapes.drawCircle({ portraitX, 15.0f }, 135.0f, portrait);
        shapes.drawFilledCircle(
            portraitX,
            68.0f,
            42.0f,
            { 0.78f, 0.84f, 0.88f, 1.0f });
        shapes.drawFilledRect(
            portraitX - 68.0f,
            -78.0f,
            136.0f,
            102.0f,
            { 0.20f, 0.24f, 0.31f, 1.0f });

        text.renderTextCentered(
            state.character == 0 ? "Knight" : "Ranger",
            portraitX,
            -160.0f,
            27.0f,
            { 0.82f, 0.93f, 1.0f, 1.0f });
    }

    class CharacterSelectScene final : public WidgeCraft::Scene {
    public:
        explicit CharacterSelectScene(DemoState& state)
            : m_state(state) {
            m_viewport.getCamera2D().setPosition({ 0.0f, 0.0f });
        }

        void onRender(WidgeCraft::WidgeCraft& app) override {
            const float width = static_cast<float>(app.getWindow().getWidth());
            const float height = static_cast<float>(app.getWindow().getHeight());
            m_viewport.setScreenRect({ 0.0f, 0.0f, width, height });
            m_viewport.getCamera2D().setZoom(std::max(
                0.45f,
                std::min(width / 1100.0f, height / 720.0f)));

            WidgeCraft::ViewportRenderOptions options;
            options.clearColor = true;
            options.color = { 0.020f, 0.030f, 0.050f, 1.0f };
            app.renderViewport(
                m_viewport,
                [this](WidgeCraft::WidgeCraft& engine) {
                    drawSelectionBackdrop(engine, m_state);
                },
                options);
        }

    private:
        DemoState& m_state;
        WidgeCraft::ModelViewport m_viewport;
    };

    class WorldScene final : public WidgeCraft::Scene {
    public:
        explicit WorldScene(DemoState& state)
            : m_state(state) {
            m_physics.setGravity({ 0.0f, -18.0f, 0.0f });
            m_physics.createBody(
                WidgeCraft::PhysicsBodyType::Static,
                { 0.0f, -0.5f, 0.0f },
                WidgeCraft::BoxCollider(
                    WidgeCraft::Vec3{ 30.0f, 0.5f, 30.0f }),
                9000);

            m_player = &m_physics.createBody(
                WidgeCraft::PhysicsBodyType::Dynamic,
                m_state.playerPosition,
                WidgeCraft::BoxCollider(
                    WidgeCraft::Vec3{ 0.55f, 0.9f, 0.55f }),
                1);
            m_player->setMass(1.0f);

            for (const EnemySpawn& spawn : EnemySpawns) {
                WidgeCraft::PhysicsBody& body = m_physics.createBody(
                    WidgeCraft::PhysicsBodyType::Static,
                    spawn.position,
                    WidgeCraft::BoxCollider(spawn.halfExtents),
                    spawn.id);
                m_enemies.push_back({ &body, spawn.halfExtents });
            }

            auto& camera = m_viewport.getCamera3D();
            camera.setPerspective(52.0f, 0.1f, 250.0f);
            updateCamera();
        }

        void onUpdate(
            WidgeCraft::WidgeCraft& app,
            float deltaTime) override {

            setViewportToWindow(app);
            auto& input = app.getInput();

            m_state.cameraDistance = std::clamp(
                m_state.cameraDistance
                    - input.scrollDelta().y * 1.25f,
                MinimumCameraDistance,
                MaximumCameraDistance);

            if (input.mouseDown(WidgeCraft::MouseButton::Right)) {
                const WidgeCraft::Vec2 mouseDelta = input.mouseDelta();
                m_state.playerYaw += mouseDelta.x * LookSensitivity;
                m_state.cameraPitch = std::clamp(
                    m_state.cameraPitch
                        + mouseDelta.y * LookSensitivity,
                    MinimumCameraPitch,
                    MaximumCameraPitch);
            }

            const WidgeCraft::Vec3 forward = horizontalForward();
            const WidgeCraft::Vec3 right{
                std::cos(m_state.playerYaw),
                0.0f,
                std::sin(m_state.playerYaw)
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
            velocity.x = movement.x * PlayerSpeed;
            velocity.z = movement.z * PlayerSpeed;
            m_player->setVelocity(velocity);

            m_physics.step(deltaTime);
            m_state.playerPosition = m_player->getPosition();
            m_state.playerMapPosition = {
                m_state.playerPosition.x,
                m_state.playerPosition.z
            };
            m_state.playerGrounded = m_player->isGrounded();

            updateCamera();
            if (input.mousePressed(WidgeCraft::MouseButton::Left)
                && !input.mouseDown(WidgeCraft::MouseButton::Right)
                && m_viewport.contains(input.mousePosition())) {
                selectEnemyAt(input.mousePosition());
            }
        }

        void onRender(WidgeCraft::WidgeCraft& app) override {
            setViewportToWindow(app);
            updateCamera();

            WidgeCraft::ViewportRenderOptions options;
            options.clearColor = true;
            options.color = { 0.018f, 0.028f, 0.040f, 1.0f };
            app.renderViewport(
                m_viewport,
                [this](WidgeCraft::WidgeCraft& engine) {
                    renderWorld(engine);
                },
                options);
        }

    private:
        struct Enemy {
            WidgeCraft::PhysicsBody* body = nullptr;
            WidgeCraft::Vec3 halfExtents{};
        };

        static constexpr float PlayerSpeed = 5.0f;
        static constexpr float LookSensitivity = 0.006f;
        static constexpr float MinimumCameraDistance = 0.0f;
        static constexpr float MaximumCameraDistance = 11.0f;
        static constexpr float FirstPersonThreshold = 0.2f;
        static constexpr float MinimumCameraPitch = -1.05f;
        static constexpr float MaximumCameraPitch = 0.78f;

        void setViewportToWindow(WidgeCraft::WidgeCraft& app) {
            m_viewport.setScreenRect({
                0.0f,
                0.0f,
                static_cast<float>(app.getWindow().getWidth()),
                static_cast<float>(app.getWindow().getHeight())
            });
        }

        WidgeCraft::Vec3 horizontalForward() const {
            return {
                std::sin(m_state.playerYaw),
                0.0f,
                -std::cos(m_state.playerYaw)
            };
        }

        WidgeCraft::Vec3 viewForward() const {
            const float pitchScale = std::cos(m_state.cameraPitch);
            return WidgeCraft::normalized({
                pitchScale * std::sin(m_state.playerYaw),
                std::sin(m_state.cameraPitch),
                -pitchScale * std::cos(m_state.playerYaw)
            });
        }

        void updateCamera() {
            const WidgeCraft::Vec3 eye =
                m_player->getPosition()
                + WidgeCraft::Vec3{ 0.0f, 0.68f, 0.0f };
            const WidgeCraft::Vec3 forward = viewForward();
            auto& camera = m_viewport.getCamera3D();
            WidgeCraft::Vec3 cameraPosition =
                eye - forward * m_state.cameraDistance;
            if (m_state.cameraDistance > FirstPersonThreshold) {
                // The demo world uses a flat ground plane. Keep the orbit
                // camera above it when looking sharply upwards.
                cameraPosition.y = std::max(cameraPosition.y, 0.2f);
            }
            camera.setPosition(cameraPosition);
            camera.setTarget(
                m_state.cameraDistance <= FirstPersonThreshold
                    ? eye + forward
                    : eye);
        }

        void selectEnemyAt(const WidgeCraft::Vec2& screenPosition) {
            const WidgeCraft::Ray ray =
                m_viewport.screenPointToRay3D(screenPosition);
            float nearestDistance = std::numeric_limits<float>::max();
            WidgeCraft::PhysicsBodyId nearestId = 0;
            for (const Enemy& enemy : m_enemies) {
                if (!enemy.body) {
                    continue;
                }
                WidgeCraft::RayHit hit;
                if (WidgeCraft::raycast(
                        ray,
                        enemy.body->getBounds(),
                        hit)
                    && hit.distance < nearestDistance) {
                    nearestDistance = hit.distance;
                    nearestId = enemy.body->getId();
                }
            }
            m_state.selectedEnemyId = nearestId;
        }

        void renderWorld(WidgeCraft::WidgeCraft& app) const {
            auto& shapes = app.getShapes3D();
            for (int coordinate = -20; coordinate <= 20; ++coordinate) {
                const bool major = coordinate == 0;
                const WidgeCraft::Color color = major
                    ? WidgeCraft::Color{ 0.28f, 0.52f, 0.68f, 0.9f }
                    : WidgeCraft::Color{ 0.10f, 0.18f, 0.24f, 0.72f };
                shapes.drawLine(
                    { static_cast<float>(coordinate), 0.01f, -20.0f },
                    { static_cast<float>(coordinate), 0.01f, 20.0f },
                    major ? 2.0f : 1.0f,
                    color);
                shapes.drawLine(
                    { -20.0f, 0.01f, static_cast<float>(coordinate) },
                    { 20.0f, 0.01f, static_cast<float>(coordinate) },
                    major ? 2.0f : 1.0f,
                    color);
            }

            if (m_state.cameraDistance > FirstPersonThreshold) {
                WidgeCraft::ShapeStyle3D playerStyle;
                playerStyle.fillColor = { 0.12f, 0.48f, 0.82f, 1.0f };
                playerStyle.edgeColor = { 0.72f, 0.92f, 1.0f, 1.0f };
                playerStyle.edgeThickness = 2.0f;
                playerStyle.edgeVisible = true;
                const WidgeCraft::AABB bounds = m_player->getBounds();
                shapes.drawBox(
                    bounds.minimum,
                    bounds.maximum,
                    playerStyle);

                const WidgeCraft::Vec3 headingStart =
                    m_player->getPosition()
                    + WidgeCraft::Vec3{ 0.0f, 0.75f, 0.0f };
                shapes.drawLine(
                    headingStart,
                    headingStart + horizontalForward() * 1.35f,
                    3.0f,
                    { 0.55f, 0.92f, 1.0f, 1.0f });
            }

            for (const Enemy& enemy : m_enemies) {
                if (!enemy.body) {
                    continue;
                }
                const bool selected =
                    enemy.body->getId() == m_state.selectedEnemyId;
                WidgeCraft::ShapeStyle3D enemyStyle;
                enemyStyle.fillColor = selected
                    ? WidgeCraft::Color{ 0.78f, 0.34f, 0.10f, 1.0f }
                    : WidgeCraft::Color{ 0.68f, 0.16f, 0.13f, 1.0f };
                enemyStyle.edgeColor = selected
                    ? WidgeCraft::Color{ 1.0f, 0.92f, 0.32f, 1.0f }
                    : WidgeCraft::Color{ 1.0f, 0.64f, 0.48f, 1.0f };
                enemyStyle.edgeThickness = selected ? 4.0f : 2.0f;
                enemyStyle.edgeVisible = true;
                const WidgeCraft::AABB bounds = enemy.body->getBounds();
                shapes.drawBox(
                    bounds.minimum,
                    bounds.maximum,
                    enemyStyle);

                if (selected) {
                    m_viewport.drawLabel3D(
                        app.getTextRenderer(),
                        "Object ID: "
                            + std::to_string(enemy.body->getId()),
                        enemy.body->getPosition()
                            + WidgeCraft::Vec3{
                                0.0f,
                                enemy.halfExtents.y + 0.45f,
                                0.0f
                            },
                        19.0f,
                        { 1.0f, 0.92f, 0.42f, 1.0f });
                }
            }
        }

        DemoState& m_state;
        WidgeCraft::ModelViewport m_viewport;
        WidgeCraft::PhysicsWorld m_physics;
        WidgeCraft::PhysicsBody* m_player = nullptr;
        std::vector<Enemy> m_enemies;
    };

    class CharacterSelectUi final : public WidgeCraft::UiScreen {
    public:
        CharacterSelectUi(
            WidgeCraft::WidgeCraft& app,
            DemoState& state)
            : UiScreen("Character Select UI")
            , m_state(state) {

            auto& panel = getRootFrame().createChildFrame("Character Panel");
            panel.setAnchor(WidgeCraft::Anchor::Center);
            panel.setSize(430.0f, 330.0f);
            panel.setBackgroundColor({ 0.045f, 0.065f, 0.095f, 0.96f });
            panel.setBorderVisible(true);
            panel.setBorderColor({ 0.30f, 0.55f, 0.78f, 1.0f });
            panel.setBorderThickness(2.0f);

            auto& title = panel.addWidget<WidgeCraft::Label>(
                "Title",
                "Choose your character");
            title.setAnchor(WidgeCraft::Anchor::TopCenter);
            title.setPosition(0.0f, 24.0f);
            title.setTextSize(31.0f);
            title.setColor({ 0.78f, 0.92f, 1.0f, 1.0f });

            m_status = &panel.addWidget<WidgeCraft::Label>(
                "Selection",
                "Selected: Knight");
            m_status->setAnchor(WidgeCraft::Anchor::TopCenter);
            m_status->setPosition(0.0f, 82.0f);
            m_status->setTextSize(18.0f);
            m_status->setColor({ 0.70f, 0.82f, 0.92f, 1.0f });

            auto& knight = panel.addWidget<WidgeCraft::Button>(
                "Knight",
                "Knight");
            knight.setAnchor(WidgeCraft::Anchor::CenterLeft);
            knight.setPosition(38.0f, 14.0f);
            knight.setSize(158.0f, 52.0f);
            knight.setTextSize(18.0f);
            knight.setOnClick([this]() {
                m_state.character = 0;
                m_status->setText("Selected: Knight");
            });

            auto& ranger = panel.addWidget<WidgeCraft::Button>(
                "Ranger",
                "Ranger");
            ranger.setAnchor(WidgeCraft::Anchor::CenterRight);
            ranger.setPosition(38.0f, 14.0f);
            ranger.setSize(158.0f, 52.0f);
            ranger.setTextSize(18.0f);
            ranger.setOnClick([this]() {
                m_state.character = 1;
                m_status->setText("Selected: Ranger");
            });

            auto& load = panel.addWidget<WidgeCraft::Button>(
                "Load World",
                "Load world");
            load.setAnchor(WidgeCraft::Anchor::BottomCenter);
            load.setPosition(0.0f, 36.0f);
            load.setSize(250.0f, 58.0f);
            load.setTextSize(20.0f);
            load.setBackgroundColor({ 0.10f, 0.38f, 0.58f, 1.0f });
            load.setHoverColor({ 0.14f, 0.50f, 0.72f, 1.0f });
            load.setOnClick([&app]() {
                app.getSceneManager().request(WorldSceneName);
                app.getUiManager().request(CombatUiName);
            });
        }

    private:
        DemoState& m_state;
        WidgeCraft::Label* m_status = nullptr;
    };

    class CombatUi final : public WidgeCraft::UiScreen {
    public:
        CombatUi(
            WidgeCraft::WidgeCraft& app,
            DemoState& state)
            : UiScreen("Combat UI")
            , m_state(state) {

            auto& root = getRootFrame();

            auto& statusPanel = root.createChildFrame("Status Panel");
            statusPanel.setAnchor(WidgeCraft::Anchor::TopLeft);
            statusPanel.setPosition(24.0f, 24.0f);
            statusPanel.setSize(390.0f, 150.0f);
            statusPanel.setBackgroundColor({ 0.040f, 0.055f, 0.078f, 0.94f });
            statusPanel.setBorderVisible(true);
            statusPanel.setBorderColor({ 0.25f, 0.48f, 0.68f, 1.0f });
            statusPanel.setBorderThickness(2.0f);

            auto& title = statusPanel.addWidget<WidgeCraft::Label>(
                "Mode",
                "Interactive world");
            title.setAnchor(WidgeCraft::Anchor::TopLeft);
            title.setPosition(18.0f, 14.0f);
            title.setTextSize(22.0f);
            title.setColor({ 0.75f, 0.91f, 1.0f, 1.0f });

            auto& movementHelp = statusPanel.addWidget<WidgeCraft::Label>(
                "Movement Help",
                "WASD move  |  Right-drag turn and look");
            movementHelp.setAnchor(WidgeCraft::Anchor::TopLeft);
            movementHelp.setPosition(18.0f, 50.0f);
            movementHelp.setTextSize(14.0f);
            movementHelp.setColor({ 0.70f, 0.80f, 0.90f, 1.0f });

            auto& selectionHelp = statusPanel.addWidget<WidgeCraft::Label>(
                "Selection Help",
                "Wheel zoom  |  Left-click an enemy");
            selectionHelp.setAnchor(WidgeCraft::Anchor::TopLeft);
            selectionHelp.setPosition(18.0f, 73.0f);
            selectionHelp.setTextSize(14.0f);
            selectionHelp.setColor({ 0.70f, 0.80f, 0.90f, 1.0f });

            m_worldStatus = &statusPanel.addWidget<WidgeCraft::Label>(
                "World Status",
                "Camera 8.0  |  Grounded");
            m_worldStatus->setAnchor(WidgeCraft::Anchor::BottomLeft);
            m_worldStatus->setPosition(18.0f, 16.0f);
            m_worldStatus->setTextSize(15.0f);
            m_worldStatus->setColor({ 0.55f, 0.94f, 0.64f, 1.0f });

            m_minimapFrame = &root.createChildFrame("Minimap Frame");
            m_minimapFrame->setAnchor(WidgeCraft::Anchor::TopRight);
            m_minimapFrame->setPosition(24.0f, 24.0f);
            m_minimapFrame->setSize(270.0f, 220.0f);
            m_minimapFrame->setBackgroundVisible(false);
            m_minimapFrame->setBorderVisible(true);
            m_minimapFrame->setBorderColor({ 0.45f, 0.78f, 0.94f, 1.0f });
            m_minimapFrame->setBorderThickness(3.0f);

            auto& mapTitle = m_minimapFrame->addWidget<WidgeCraft::Label>(
                "Map Title",
                "MINIMAP");
            mapTitle.setAnchor(WidgeCraft::Anchor::TopCenter);
            mapTitle.setPosition(0.0f, 10.0f);
            mapTitle.setTextSize(15.0f);
            mapTitle.setColor({ 0.78f, 0.92f, 1.0f, 1.0f });

            m_minimap = &createSceneView("Minimap", *m_minimapFrame);
            m_minimap->setInsets(4.0f, 4.0f, 4.0f, 34.0f);
            m_minimap->setClearColor({ 0.018f, 0.052f, 0.065f, 1.0f });
            m_minimap->getViewport().getCamera2D().setZoom(7.0f);
            m_minimap->setRenderCallback(
                [this](
                    WidgeCraft::WidgeCraft& engine,
                    const WidgeCraft::SceneView& view) {
                    renderMinimap(engine, view);
                });

            auto& controls = root.createChildFrame("Combat Controls");
            controls.setAnchor(WidgeCraft::Anchor::BottomCenter);
            controls.setPosition(0.0f, 24.0f);
            controls.setSize(430.0f, 82.0f);
            controls.setBackgroundColor({ 0.040f, 0.055f, 0.078f, 0.94f });
            controls.setBorderVisible(true);
            controls.setBorderColor({ 0.25f, 0.48f, 0.68f, 1.0f });

            auto& resizeMap = controls.addWidget<WidgeCraft::Button>(
                "Resize Map",
                "Expand map");
            resizeMap.setAnchor(WidgeCraft::Anchor::CenterLeft);
            resizeMap.setPosition(16.0f, 0.0f);
            resizeMap.setSize(188.0f, 48.0f);
            resizeMap.setTextSize(17.0f);
            auto* resizeMapButton = &resizeMap;
            resizeMap.setOnClick([this, resizeMapButton]() {
                m_mapExpanded = !m_mapExpanded;
                m_minimapFrame->setSize(
                    m_mapExpanded ? 410.0f : 270.0f,
                    m_mapExpanded ? 330.0f : 220.0f);
                resizeMapButton->setText(
                    m_mapExpanded ? "Shrink map" : "Expand map");
            });

            auto& back = controls.addWidget<WidgeCraft::Button>(
                "Back",
                "Character select");
            back.setAnchor(WidgeCraft::Anchor::CenterRight);
            back.setPosition(16.0f, 0.0f);
            back.setSize(188.0f, 48.0f);
            back.setTextSize(17.0f);
            back.setOnClick([&app]() {
                app.getSceneManager().request(CharacterSceneName);
                app.getUiManager().request(CharacterUiName);
            });
        }

        void onUpdate(
            WidgeCraft::WidgeCraft& app,
            float deltaTime) override {
            (void)app;
            (void)deltaTime;
            m_minimap->getViewport().getCamera2D().setPosition(
                m_state.playerMapPosition);

            const int distanceTenths = static_cast<int>(
                std::round(m_state.cameraDistance * 10.0f));
            std::string status = m_state.cameraDistance <= 0.2f
                ? "First person"
                : "Camera "
                    + std::to_string(distanceTenths / 10)
                    + "."
                    + std::to_string(distanceTenths % 10);
            status += m_state.playerGrounded
                ? "  |  Grounded"
                : "  |  Falling";
            if (m_state.selectedEnemyId != 0) {
                status += "  |  Selected "
                    + std::to_string(m_state.selectedEnemyId);
            }
            m_worldStatus->setText(std::move(status));
        }

    private:
        void renderMinimap(
            WidgeCraft::WidgeCraft& app,
            const WidgeCraft::SceneView& view) const {

            auto& shapes = app.getShapes2D();
            for (int coordinate = -50; coordinate <= 50; coordinate += 5) {
                const bool major = coordinate % 10 == 0;
                const WidgeCraft::Color color = major
                    ? WidgeCraft::Color{ 0.16f, 0.36f, 0.40f, 0.88f }
                    : WidgeCraft::Color{ 0.09f, 0.22f, 0.25f, 0.72f };
                shapes.drawLine(
                    static_cast<float>(coordinate), -50.0f,
                    static_cast<float>(coordinate), 50.0f,
                    major ? 0.20f : 0.10f,
                    color);
                shapes.drawLine(
                    -50.0f, static_cast<float>(coordinate),
                    50.0f, static_cast<float>(coordinate),
                    major ? 0.20f : 0.10f,
                    color);
            }

            WidgeCraft::ShapeStyle2D player;
            player.fillColor = { 0.35f, 0.88f, 1.0f, 1.0f };
            player.edgeColor = WidgeCraft::Colors::White;
            player.edgeThickness = 0.35f;
            player.edgeVisible = true;
            shapes.drawCircle(m_state.playerMapPosition, 1.2f, player);
            const WidgeCraft::Vec2 heading{
                std::sin(m_state.playerYaw),
                -std::cos(m_state.playerYaw)
            };
            shapes.drawLine(
                m_state.playerMapPosition.x,
                m_state.playerMapPosition.y,
                m_state.playerMapPosition.x + heading.x * 2.4f,
                m_state.playerMapPosition.y + heading.y * 2.4f,
                0.38f,
                { 0.70f, 0.95f, 1.0f, 1.0f });

            for (const EnemySpawn& spawn : EnemySpawns) {
                const bool selected =
                    spawn.id == m_state.selectedEnemyId;
                WidgeCraft::ShapeStyle2D enemy;
                enemy.fillColor = selected
                    ? WidgeCraft::Color{ 1.0f, 0.82f, 0.18f, 1.0f }
                    : WidgeCraft::Color{ 1.0f, 0.28f, 0.18f, 1.0f };
                enemy.edgeColor = WidgeCraft::Colors::White;
                enemy.edgeThickness = 0.28f;
                enemy.edgeVisible = selected;
                shapes.drawCircle(
                    { spawn.position.x, spawn.position.z },
                    std::max(spawn.halfExtents.x, spawn.halfExtents.z),
                    enemy);
            }

            view.getViewport().drawLabel2D(
                app.getTextRenderer(),
                "You",
                m_state.playerMapPosition,
                13.0f,
                { 0.78f, 0.94f, 1.0f, 1.0f },
                { 10.0f, 8.0f });
        }

        DemoState& m_state;
        WidgeCraft::Frame* m_minimapFrame = nullptr;
        WidgeCraft::SceneView* m_minimap = nullptr;
        WidgeCraft::Label* m_worldStatus = nullptr;
        bool m_mapExpanded = false;
    };

} // namespace

int main(int argumentCount, char** arguments) {
    try {
        WidgeCraft::WidgeCraft app(
            "WidgeCraft Interactive World Sandbox",
            1100,
            720);
        app.setClearColor({ 0.020f, 0.028f, 0.044f, 1.0f });

        DemoState state;
        app.getSceneManager().emplace<CharacterSelectScene>(
            CharacterSceneName,
            state);
        app.getSceneManager().emplace<WorldScene>(
            WorldSceneName,
            state);
        app.getUiManager().emplace<CharacterSelectUi>(
            CharacterUiName,
            app,
            state);
        app.getUiManager().emplace<CombatUi>(
            CombatUiName,
            app,
            state);

        const bool startInWorld = argumentCount > 1
            && std::string(arguments[1]) == "--world";
        const char* initialScene = startInWorld
            ? WorldSceneName
            : CharacterSceneName;
        const char* initialUi = startInWorld
            ? CombatUiName
            : CharacterUiName;
        if (!app.getSceneManager().activate(initialScene)
            || !app.getUiManager().activate(initialUi)) {
            throw std::runtime_error(
                "Failed to activate the initial scene and UI");
        }

        app.Run(60);
    } catch (const std::exception& exception) {
        std::cerr << "WidgeCraft management sandbox failed: "
                  << exception.what() << '\n';
        return 1;
    }

    return 0;
}
