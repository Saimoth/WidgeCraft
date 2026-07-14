// GLAD must be included before GLFW.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "WidgeCraft/WidgeCraft.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    constexpr float kPi = 3.14159265358979323846f;

    struct Cube {
        WidgeCraft::Vec3 center{};
        float size = 1.0f;
        WidgeCraft::Color color = WidgeCraft::Colors::White;
        int paletteIndex = 0;
    };

    struct Camera {
        WidgeCraft::Vec3 position{ 0.0f, 0.5f, 8.0f };
        float yaw = -kPi * 0.5f;
        float pitch = 0.0f;

        WidgeCraft::Vec3 forward() const {
            const float cosPitch = std::cos(pitch);
            return WidgeCraft::normalized({
                cosPitch * std::cos(yaw),
                std::sin(pitch),
                cosPitch * std::sin(yaw)
            });
        }
    };

    WidgeCraft::Mat4 makePerspective(
        float verticalFieldOfViewRadians,
        float aspectRatio,
        float nearPlane,
        float farPlane) {

        WidgeCraft::Mat4 result{};
        const float safeAspect = std::max(aspectRatio, 0.001f);
        const float focalLength = 1.0f / std::tan(verticalFieldOfViewRadians * 0.5f);

        result(0, 0) = focalLength / safeAspect;
        result(1, 1) = focalLength;
        result(2, 2) = (farPlane + nearPlane) / (nearPlane - farPlane);
        result(2, 3) = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
        result(3, 2) = -1.0f;
        return result;
    }

    WidgeCraft::Mat4 makeView(
        const WidgeCraft::Vec3& eye,
        const WidgeCraft::Vec3& forward) {

        const WidgeCraft::Vec3 worldUp{ 0.0f, 1.0f, 0.0f };
        const WidgeCraft::Vec3 viewForward = WidgeCraft::normalized(forward);
        const WidgeCraft::Vec3 viewRight = WidgeCraft::normalized(
            WidgeCraft::cross(viewForward, worldUp));
        const WidgeCraft::Vec3 viewUp = WidgeCraft::cross(viewRight, viewForward);

        WidgeCraft::Mat4 result = WidgeCraft::Mat4::identity();
        result(0, 0) = viewRight.x;
        result(0, 1) = viewRight.y;
        result(0, 2) = viewRight.z;
        result(0, 3) = -WidgeCraft::dot(viewRight, eye);

        result(1, 0) = viewUp.x;
        result(1, 1) = viewUp.y;
        result(1, 2) = viewUp.z;
        result(1, 3) = -WidgeCraft::dot(viewUp, eye);

        result(2, 0) = -viewForward.x;
        result(2, 1) = -viewForward.y;
        result(2, 2) = -viewForward.z;
        result(2, 3) = WidgeCraft::dot(viewForward, eye);
        return result;
    }

    WidgeCraft::Color shade(
        const WidgeCraft::Color& color,
        float amount) {

        return {
            std::clamp(color.r * amount, 0.0f, 1.0f),
            std::clamp(color.g * amount, 0.0f, 1.0f),
            std::clamp(color.b * amount, 0.0f, 1.0f),
            color.a
        };
    }

    class CubeRenderer {
    public:
        CubeRenderer() {
            createShader();
            createBuffers();
            m_vertices.reserve(36U * 16U);
        }

        ~CubeRenderer() {
            if (m_vbo != 0) {
                glDeleteBuffers(1, &m_vbo);
            }
            if (m_vao != 0) {
                glDeleteVertexArrays(1, &m_vao);
            }
            if (m_shader != 0) {
                glDeleteProgram(m_shader);
            }
        }

        CubeRenderer(const CubeRenderer&) = delete;
        CubeRenderer& operator=(const CubeRenderer&) = delete;

        void draw(
            const std::vector<Cube>& cubes,
            const WidgeCraft::Mat4& viewProjection) {

            m_vertices.clear();
            m_vertices.reserve(cubes.size() * 36U);
            for (const Cube& cube : cubes) {
                appendCube(cube);
            }

            if (m_vertices.empty()) {
                return;
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glDisable(GL_CULL_FACE);

            glUseProgram(m_shader);
            glUniformMatrix4fv(
                m_uniformViewProjection,
                1,
                GL_FALSE,
                viewProjection.values.data());

            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
                m_vertices.data(),
                GL_DYNAMIC_DRAW);
            glDrawArrays(
                GL_TRIANGLES,
                0,
                static_cast<GLsizei>(m_vertices.size()));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);
            glDisable(GL_DEPTH_TEST);
        }

    private:
        struct Vertex {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;
        };

        static unsigned int compileShader(
            unsigned int type,
            std::string_view source) {

            const unsigned int shader = glCreateShader(type);
            const char* sourceData = source.data();
            const int sourceLength = static_cast<int>(source.size());
            glShaderSource(shader, 1, &sourceData, &sourceLength);
            glCompileShader(shader);

            int status = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE) {
                int logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(
                    static_cast<std::size_t>(std::max(logLength, 1)),
                    '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                glDeleteShader(shader);
                throw std::runtime_error(
                    "3D sandbox shader compilation failed: " + log);
            }
            return shader;
        }

        void createShader() {
            constexpr std::string_view vertexSource = R"(
                #version 330 core
                layout (location = 0) in vec3 aPosition;
                layout (location = 1) in vec4 aColor;

                uniform mat4 uViewProjection;

                out vec4 vColor;

                void main() {
                    gl_Position = uViewProjection * vec4(aPosition, 1.0);
                    vColor = aColor;
                }
            )";

            constexpr std::string_view fragmentSource = R"(
                #version 330 core
                in vec4 vColor;
                out vec4 fragColor;

                void main() {
                    fragColor = vColor;
                }
            )";

            const unsigned int vertexShader = compileShader(
                GL_VERTEX_SHADER,
                vertexSource);
            const unsigned int fragmentShader = compileShader(
                GL_FRAGMENT_SHADER,
                fragmentSource);

            m_shader = glCreateProgram();
            glAttachShader(m_shader, vertexShader);
            glAttachShader(m_shader, fragmentShader);
            glLinkProgram(m_shader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            int status = GL_FALSE;
            glGetProgramiv(m_shader, GL_LINK_STATUS, &status);
            if (status == GL_FALSE) {
                int logLength = 0;
                glGetProgramiv(m_shader, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(
                    static_cast<std::size_t>(std::max(logLength, 1)),
                    '\0');
                glGetProgramInfoLog(
                    m_shader,
                    logLength,
                    nullptr,
                    log.data());
                glDeleteProgram(m_shader);
                m_shader = 0;
                throw std::runtime_error(
                    "3D sandbox shader linking failed: " + log);
            }

            m_uniformViewProjection = glGetUniformLocation(
                m_shader,
                "uViewProjection");
        }

        void createBuffers() {
            glGenVertexArrays(1, &m_vao);
            glGenBuffers(1, &m_vbo);

            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(offsetof(Vertex, x)));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                1,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(offsetof(Vertex, r)));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void addVertex(
            const WidgeCraft::Vec3& position,
            const WidgeCraft::Color& color) {

            m_vertices.push_back({
                position.x,
                position.y,
                position.z,
                color.r,
                color.g,
                color.b,
                color.a
            });
        }

        void appendFace(
            const std::array<WidgeCraft::Vec3, 8>& corners,
            int a,
            int b,
            int c,
            int d,
            const WidgeCraft::Color& color) {

            addVertex(corners[static_cast<std::size_t>(a)], color);
            addVertex(corners[static_cast<std::size_t>(b)], color);
            addVertex(corners[static_cast<std::size_t>(c)], color);
            addVertex(corners[static_cast<std::size_t>(a)], color);
            addVertex(corners[static_cast<std::size_t>(c)], color);
            addVertex(corners[static_cast<std::size_t>(d)], color);
        }

        void appendCube(const Cube& cube) {
            const float halfSize = cube.size * 0.5f;
            const WidgeCraft::Vec3 minimum = cube.center - WidgeCraft::Vec3{
                halfSize,
                halfSize,
                halfSize
            };
            const WidgeCraft::Vec3 maximum = cube.center + WidgeCraft::Vec3{
                halfSize,
                halfSize,
                halfSize
            };

            const std::array<WidgeCraft::Vec3, 8> corners{
                WidgeCraft::Vec3{ minimum.x, minimum.y, minimum.z },
                WidgeCraft::Vec3{ maximum.x, minimum.y, minimum.z },
                WidgeCraft::Vec3{ maximum.x, maximum.y, minimum.z },
                WidgeCraft::Vec3{ minimum.x, maximum.y, minimum.z },
                WidgeCraft::Vec3{ minimum.x, minimum.y, maximum.z },
                WidgeCraft::Vec3{ maximum.x, minimum.y, maximum.z },
                WidgeCraft::Vec3{ maximum.x, maximum.y, maximum.z },
                WidgeCraft::Vec3{ minimum.x, maximum.y, maximum.z }
            };

            appendFace(corners, 0, 3, 2, 1, shade(cube.color, 0.58f));
            appendFace(corners, 4, 5, 6, 7, shade(cube.color, 1.00f));
            appendFace(corners, 0, 4, 7, 3, shade(cube.color, 0.72f));
            appendFace(corners, 1, 2, 6, 5, shade(cube.color, 0.86f));
            appendFace(corners, 0, 1, 5, 4, shade(cube.color, 0.64f));
            appendFace(corners, 3, 7, 6, 2, shade(cube.color, 0.94f));
        }

        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_shader = 0;
        int m_uniformViewProjection = -1;
        std::vector<Vertex> m_vertices;
    };

    WidgeCraft::AABB boundsFor(const Cube& cube) {
        const float halfSize = cube.size * 0.5f;
        const WidgeCraft::Vec3 halfExtents{
            halfSize,
            halfSize,
            halfSize
        };
        return {
            cube.center - halfExtents,
            cube.center + halfExtents
        };
    }

} // namespace

int main() {
    try {
        WidgeCraft::WidgeCraft app(
            "WidgeCraft 3D Raycast Sandbox",
            1100,
            720);
        app.setClearColor({ 0.025f, 0.035f, 0.055f, 1.0f });

        CubeRenderer cubeRenderer;
        Camera camera;

        const std::array<WidgeCraft::Color, 8> palette{
            WidgeCraft::Color{ 0.22f, 0.66f, 0.96f, 1.0f },
            WidgeCraft::Color{ 0.95f, 0.42f, 0.35f, 1.0f },
            WidgeCraft::Color{ 0.35f, 0.84f, 0.54f, 1.0f },
            WidgeCraft::Color{ 0.92f, 0.72f, 0.24f, 1.0f },
            WidgeCraft::Color{ 0.68f, 0.43f, 0.92f, 1.0f },
            WidgeCraft::Color{ 0.25f, 0.86f, 0.84f, 1.0f },
            WidgeCraft::Color{ 0.95f, 0.52f, 0.79f, 1.0f },
            WidgeCraft::Color{ 0.88f, 0.90f, 0.96f, 1.0f }
        };

        std::vector<Cube> cubes{
            { { -3.0f,  0.0f,  -1.5f }, 1.35f, palette[0], 0 },
            { {  0.0f,  1.2f,  -3.0f }, 1.70f, palette[1], 1 },
            { {  3.1f, -0.7f,  -2.4f }, 1.25f, palette[2], 2 },
            { { -1.8f,  2.4f,  -6.3f }, 1.90f, palette[3], 3 },
            { {  2.3f,  1.8f,  -7.2f }, 1.45f, palette[4], 4 },
            { {  0.2f, -2.0f,  -5.4f }, 1.55f, palette[5], 5 },
            { {  4.3f,  2.9f, -10.0f }, 2.10f, palette[6], 6 },
            { { -4.0f, -2.5f,  -9.0f }, 1.75f, palette[7], 7 }
        };

        WidgeCraft::Mat4 view = WidgeCraft::Mat4::identity();
        WidgeCraft::Mat4 projection = WidgeCraft::Mat4::identity();
        std::string statusText = "Left-click a cube to cycle its colour";

        app.setUpdateCallback([&](WidgeCraft::WidgeCraft& engine) {
            const WidgeCraft::Input& input = engine.getInput();
            const float deltaTime = engine.getDeltaTime();
            const WidgeCraft::Vec3 worldUp{ 0.0f, 1.0f, 0.0f };

            if (input.keyPressed(GLFW_KEY_ESCAPE)) {
                engine.Stop();
                return;
            }

            if (input.mouseDown(WidgeCraft::MouseButton::Right)) {
                const WidgeCraft::Vec2 mouseDelta = input.mouseDelta();
                constexpr float lookSensitivity = 0.0040f;
                camera.yaw += mouseDelta.x * lookSensitivity;
                camera.pitch = std::clamp(
                    camera.pitch + mouseDelta.y * lookSensitivity,
                    -1.48f,
                    1.48f);
            }

            const WidgeCraft::Vec3 forward = camera.forward();
            const WidgeCraft::Vec3 right = WidgeCraft::normalized(
                WidgeCraft::cross(forward, worldUp));
            WidgeCraft::Vec3 movement{};

            if (input.keyDown(GLFW_KEY_W)) {
                movement += forward;
            }
            if (input.keyDown(GLFW_KEY_S)) {
                movement -= forward;
            }
            if (input.keyDown(GLFW_KEY_D)) {
                movement += right;
            }
            if (input.keyDown(GLFW_KEY_A)) {
                movement -= right;
            }

            if (WidgeCraft::lengthSquared(movement) > 0.0f) {
                const bool fast = input.keyDown(GLFW_KEY_LEFT_SHIFT)
                    || input.keyDown(GLFW_KEY_RIGHT_SHIFT);
                const float movementSpeed = fast ? 12.0f : 5.0f;
                camera.position += WidgeCraft::normalized(movement)
                    * movementSpeed
                    * deltaTime;
            }

            view = makeView(camera.position, camera.forward());
            projection = makePerspective(
                kPi / 3.0f,
                engine.getWindow().getAspectRatio(),
                0.1f,
                100.0f);

            if (input.mousePressed(WidgeCraft::MouseButton::Left)
                && !input.mouseDown(WidgeCraft::MouseButton::Right)) {

                const WidgeCraft::Vec2 mouse = input.mousePosition();
                const WidgeCraft::Ray ray = WidgeCraft::screenPointToRay(
                    mouse.x,
                    mouse.y,
                    static_cast<float>(engine.getWindow().getWidth()),
                    static_cast<float>(engine.getWindow().getHeight()),
                    view,
                    projection);

                float nearestDistance = std::numeric_limits<float>::max();
                std::size_t nearestCube = cubes.size();
                for (std::size_t index = 0; index < cubes.size(); ++index) {
                    WidgeCraft::RayHit hit{};
                    if (WidgeCraft::raycast(
                            ray,
                            boundsFor(cubes[index]),
                            hit)
                        && hit.distance < nearestDistance) {

                        nearestDistance = hit.distance;
                        nearestCube = index;
                    }
                }

                if (nearestCube < cubes.size()) {
                    Cube& cube = cubes[nearestCube];
                    cube.paletteIndex = (cube.paletteIndex + 1)
                        % static_cast<int>(palette.size());
                    cube.color = palette[
                        static_cast<std::size_t>(cube.paletteIndex)];
                    statusText = "Ray hit cube "
                        + std::to_string(nearestCube + 1U)
                        + " at distance "
                        + std::to_string(nearestDistance).substr(0, 4);
                } else {
                    statusText = "Ray missed every cube";
                }
            }
        });

        app.setRenderCallback([&](WidgeCraft::WidgeCraft& engine) {
            cubeRenderer.draw(cubes, projection * view);

            auto& shapes = engine.getShapeRenderer();
            auto& text = engine.getTextRenderer();
            const float height = static_cast<float>(
                engine.getWindow().getHeight());

            text.renderText(
                "WidgeCraft 3D Raycast Sandbox",
                20.0f,
                height - 38.0f,
                24.0f,
                { 0.76f, 0.90f, 1.0f, 1.0f });
            text.renderText(
                "WASD fly  |  Shift faster  |  Hold RMB and drag to look  |  LMB changes cube colour  |  Esc exits",
                20.0f,
                height - 66.0f,
                14.0f,
                { 0.72f, 0.76f, 0.84f, 1.0f });
            text.renderText(
                statusText,
                20.0f,
                24.0f,
                16.0f,
                { 0.70f, 0.88f, 0.72f, 1.0f });

            const WidgeCraft::Vec2 mouse = engine.getInput().mousePosition();
            constexpr float crosshairRadius = 7.0f;
            shapes.drawLine(
                mouse.x - crosshairRadius,
                mouse.y,
                mouse.x + crosshairRadius,
                mouse.y,
                1.5f,
                { 0.95f, 0.95f, 1.0f, 0.9f });
            shapes.drawLine(
                mouse.x,
                mouse.y - crosshairRadius,
                mouse.x,
                mouse.y + crosshairRadius,
                1.5f,
                { 0.95f, 0.95f, 1.0f, 0.9f });
        });

        app.Run(60);
    } catch (const std::exception& exception) {
        std::fprintf(stderr, "WidgeCraft 3D sandbox failed: %s\n", exception.what());
        return 1;
    }

    return 0;
}
