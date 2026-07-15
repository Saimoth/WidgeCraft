#pragma once

#include "WidgeCraft/primitives/Shapes2D.hpp"
#include "WidgeCraft/primitives/Shapes3D.hpp"
#include "WidgeCraft/primitives/TextRenderer.hpp"
#include "WidgeCraft/scene/Camera2D.hpp"
#include "WidgeCraft/scene/Camera3D.hpp"
#include "WidgeCraft/scene/Raycast.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace WidgeCraft {

    struct ProjectedPoint3D {
        Vec2 screen{};
        float depth = 1.0f;
        bool inFront = false;
        bool visible = false;
    };

    class ModelViewport {
    public:
        explicit ModelViewport(const Rect& screenRect = Rect{ 0.0f, 0.0f, 1.0f, 1.0f })
            : m_screenRect(screenRect) {
            sanitizeRect();
        }

        void setScreenRect(const Rect& screenRect) {
            m_screenRect = screenRect;
            sanitizeRect();
        }

        Rect getScreenRect() const { return m_screenRect; }

        float getAspectRatio() const {
            return m_screenRect.height > 0.0f
                ? m_screenRect.width / m_screenRect.height
                : 1.0f;
        }

        bool contains(const Vec2& screenPosition) const {
            return m_screenRect.contains(screenPosition);
        }

        Camera2D& getCamera2D() { return m_camera2D; }
        const Camera2D& getCamera2D() const { return m_camera2D; }

        Camera3D& getCamera3D() { return m_camera3D; }
        const Camera3D& getCamera3D() const { return m_camera3D; }

        // Draw distance is the camera far plane. Pair it with ObjectManager's
        // CPU culling distance to avoid queuing objects that the GPU will clip.
        void setDrawDistance(float distance) {
            m_camera3D.setFarPlane(distance);
        }
        float getDrawDistance() const {
            return m_camera3D.getFarPlane();
        }

        Vec2 worldToScreen2D(const Vec2& worldPosition) const {
            return m_camera2D.worldToScreen(worldPosition, m_screenRect);
        }

        Vec2 screenToWorld2D(const Vec2& screenPosition) const {
            return m_camera2D.screenToWorld(screenPosition, m_screenRect);
        }

        Vec2 getShapes2DOffset() const {
            return m_camera2D.getRenderOffset(m_screenRect);
        }

        float getShapes2DScale() const {
            return m_camera2D.getZoom();
        }

        Mat4 getViewMatrix3D() const {
            return m_camera3D.getViewMatrix();
        }

        Mat4 getProjectionMatrix3D() const {
            return m_camera3D.getProjectionMatrix(getAspectRatio());
        }

        Mat4 getViewProjection3D() const {
            return m_camera3D.getViewProjection(getAspectRatio());
        }

        Ray screenPointToRay3D(const Vec2& screenPosition) const {
            const Vec2 local{
                screenPosition.x - m_screenRect.x,
                screenPosition.y - m_screenRect.y
            };
            return screenPointToRay(
                local.x,
                local.y,
                m_screenRect.width,
                m_screenRect.height,
                getViewMatrix3D(),
                getProjectionMatrix3D());
        }

        ProjectedPoint3D projectWorldToScreen3D(
            const Vec3& worldPosition) const {

            ProjectedPoint3D result{};
            const Vec4 clip = getViewProjection3D()
                * Vec4{
                    worldPosition.x,
                    worldPosition.y,
                    worldPosition.z,
                    1.0f
                };

            if (std::abs(clip.w) <= 1.0e-6f) {
                return result;
            }

            result.inFront = clip.w > 0.0f;
            if (!result.inFront) {
                return result;
            }

            const float inverseW = 1.0f / clip.w;
            const Vec3 ndc{
                clip.x * inverseW,
                clip.y * inverseW,
                clip.z * inverseW
            };

            result.screen = {
                m_screenRect.x
                    + (ndc.x * 0.5f + 0.5f) * m_screenRect.width,
                m_screenRect.y
                    + (ndc.y * 0.5f + 0.5f) * m_screenRect.height
            };
            result.depth = ndc.z * 0.5f + 0.5f;
            result.visible =
                ndc.x >= -1.0f && ndc.x <= 1.0f
                && ndc.y >= -1.0f && ndc.y <= 1.0f
                && ndc.z >= -1.0f && ndc.z <= 1.0f;
            return result;
        }

        void configureRenderers(
            Shapes2D& shapes2D,
            Shapes3D& shapes3D) const {

            shapes2D.setTransform(
                getShapes2DOffset(),
                getShapes2DScale());
            shapes3D.setViewProjection(getViewProjection3D());
        }

        void drawWorldText2D(
            TextRenderer& textRenderer,
            const std::string& text,
            const Vec2& worldBaseline,
            float worldSize,
            Color color = Colors::White) const {

            const Vec2 screen = worldToScreen2D(worldBaseline);
            textRenderer.renderText(
                text,
                screen.x,
                screen.y,
                std::max(worldSize * m_camera2D.getZoom(), 0.0f),
                color);
        }

        void drawLabel2D(
            TextRenderer& textRenderer,
            const std::string& text,
            const Vec2& worldPosition,
            float sizePixels,
            Color color = Colors::White,
            const Vec2& pixelOffset = {}) const {

            const Vec2 screen = worldToScreen2D(worldPosition) + pixelOffset;
            textRenderer.renderText(
                text,
                screen.x,
                screen.y,
                sizePixels,
                color);
        }

        bool drawLabel3D(
            TextRenderer& textRenderer,
            const std::string& text,
            const Vec3& worldPosition,
            float sizePixels,
            Color color = Colors::White,
            const Vec2& pixelOffset = {}) const {

            const ProjectedPoint3D projected =
                projectWorldToScreen3D(worldPosition);
            if (!projected.visible) {
                return false;
            }

            textRenderer.renderText(
                text,
                projected.screen.x + pixelOffset.x,
                projected.screen.y + pixelOffset.y,
                sizePixels,
                color);
            return true;
        }

        bool drawWorldText3D(
            TextRenderer& textRenderer,
            const std::string& text,
            const Vec3& worldPosition,
            float worldHeight,
            Color color = Colors::White,
            const Vec2& pixelOffset = {}) const {

            const ProjectedPoint3D baseline =
                projectWorldToScreen3D(worldPosition);
            if (!baseline.visible) {
                return false;
            }

            Vec3 up = m_camera3D.getUp();
            if (lengthSquared(up) <= 1.0e-12f) {
                up = { 0.0f, 1.0f, 0.0f };
            } else {
                up = normalized(up);
            }

            const ProjectedPoint3D top = projectWorldToScreen3D(
                worldPosition + up * worldHeight);
            if (!top.inFront) {
                return false;
            }

            const float pixelHeight = length(top.screen - baseline.screen);
            if (pixelHeight <= 0.0f) {
                return false;
            }

            textRenderer.renderText(
                text,
                baseline.screen.x + pixelOffset.x,
                baseline.screen.y + pixelOffset.y,
                pixelHeight,
                color);
            return true;
        }

    private:
        void sanitizeRect() {
            m_screenRect.width = std::max(m_screenRect.width, 1.0f);
            m_screenRect.height = std::max(m_screenRect.height, 1.0f);
        }

        Rect m_screenRect;
        Camera2D m_camera2D;
        Camera3D m_camera3D;
    };

} // namespace WidgeCraft
