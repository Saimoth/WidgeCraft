#pragma once

#include "WidgeCraft/model/Mesh.hpp"
#include "WidgeCraft/physics/PhysicsWorld.hpp"
#include "WidgeCraft/primitives/Shapes3D.hpp"
#include "WidgeCraft/primitives/TextRenderer.hpp"
#include "WidgeCraft/scene/ModelViewport.hpp"
#include "WidgeCraft/scene/Raycast.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace WidgeCraft {

    using ObjectId = std::uint64_t;

    struct ObjectAttributes {
        bool enabled = true;
        bool rendered = true;
        bool interactable = true;
        bool collidable = false;
        bool fixed = true;
        bool gravity = false;
        bool trigger = false;
        bool selectable = true;
        bool labelVisible = false;
    };

    struct ObjectRenderStats {
        std::size_t considered = 0;
        std::size_t rendered = 0;
        std::size_t distanceCulled = 0;
        std::size_t hidden = 0;
        std::size_t trianglesQueued = 0;
        std::size_t labelsQueued = 0;
        std::size_t vertexBytesQueued = 0;
        double cpuMilliseconds = 0.0;
    };

    struct ObjectRayHit {
        ObjectId objectId = 0;
        RayHit hit{};
    };

    class SceneObject {
    public:
        ObjectId getId() const { return m_id; }
        const std::string& getName() const { return m_name; }
        void setName(std::string name) { m_name = std::move(name); }

        const Transform3D& getTransform() const { return m_transform; }
        void setTransform(const Transform3D& transform);
        void setPosition(const Vec3& position);
        void setRotation(const Vec3& rotation);
        void setScale(const Vec3& scale);

        ObjectAttributes& getAttributes() { return m_attributes; }
        const ObjectAttributes& getAttributes() const {
            return m_attributes;
        }

        void setMesh(std::shared_ptr<const Mesh3D> mesh);
        std::shared_ptr<const Mesh3D> getMesh() const { return m_mesh; }

        ShapeStyle3D& getStyle() { return m_style; }
        const ShapeStyle3D& getStyle() const { return m_style; }

        AABB getWorldBounds() const;
        PhysicsBodyId getPhysicsBodyId() const {
            return m_physicsBodyId;
        }

    private:
        friend class ObjectManager;

        SceneObject(
            ObjectId id,
            std::string name,
            std::shared_ptr<const Mesh3D> mesh,
            const Transform3D& transform);

        ObjectId m_id = 0;
        std::string m_name;
        std::shared_ptr<const Mesh3D> m_mesh;
        Transform3D m_transform;
        ShapeStyle3D m_style;
        ObjectAttributes m_attributes;
        PhysicsBodyId m_physicsBodyId = 0;
        bool m_transformDirty = true;
    };

    class ObjectManager {
    public:
        explicit ObjectManager(PhysicsWorld* physicsWorld = nullptr)
            : m_physicsWorld(physicsWorld) {
        }

        SceneObject& createMeshObject(
            std::string name,
            std::shared_ptr<const Mesh3D> mesh,
            const Transform3D& transform = {},
            ObjectId requestedId = 0);
        SceneObject& createBoxObject(
            std::string name,
            const Vec3& halfExtents,
            const Transform3D& transform = {},
            ObjectId requestedId = 0);

        SceneObject* find(ObjectId id);
        const SceneObject* find(ObjectId id) const;
        bool remove(ObjectId id);
        void clear();

        const std::vector<std::unique_ptr<SceneObject>>& getObjects() const {
            return m_objects;
        }
        std::size_t getObjectCount() const { return m_objects.size(); }

        void setDrawDistance(float distance);
        float getDrawDistance() const { return m_drawDistance; }

        ObjectRenderStats render(
            Shapes3D& shapes,
            const Vec3& viewPosition,
            const ModelViewport* viewport = nullptr,
            TextRenderer* textRenderer = nullptr) const;

        bool raycastInteractable(
            const Ray& ray,
            ObjectRayHit& result,
            float maximumDistance = 0.0f) const;
        std::vector<ObjectId> queryTriggers(const AABB& bounds) const;

        void setPhysicsWorld(PhysicsWorld* physicsWorld);
        PhysicsWorld* getPhysicsWorld() const { return m_physicsWorld; }
        void preparePhysics();
        void syncFromPhysics();
        PhysicsBody* getPhysicsBody(SceneObject& object);
        const PhysicsBody* getPhysicsBody(const SceneObject& object) const;
        std::vector<ObjectId> getTriggeredObjects(
            PhysicsBodyId otherBodyId) const;

        std::size_t getApproximateMemoryUsageBytes() const;

    private:
        ObjectId allocateId();
        void detachPhysics(SceneObject& object);

        std::vector<std::unique_ptr<SceneObject>> m_objects;
        PhysicsWorld* m_physicsWorld = nullptr;
        ObjectId m_nextId = 1;
        float m_drawDistance = 1000.0f;
    };

} // namespace WidgeCraft
