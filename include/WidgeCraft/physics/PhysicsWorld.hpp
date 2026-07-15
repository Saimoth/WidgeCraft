#pragma once

#include "WidgeCraft/physics/Collider.hpp"
#include "WidgeCraft/scene/Raycast.hpp"
#include "WidgeCraft/terrain/HeightMap.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace WidgeCraft {

    using PhysicsBodyId = std::uint64_t;

    enum class PhysicsBodyType {
        Static,
        Dynamic
    };

    class PhysicsBody {
    public:
        PhysicsBodyId getId() const { return m_id; }

        PhysicsBodyType getType() const { return m_type; }
        void setType(PhysicsBodyType type);
        bool isStatic() const { return m_type == PhysicsBodyType::Static; }
        bool isDynamic() const { return m_type == PhysicsBodyType::Dynamic; }

        Vec3 getPosition() const { return m_position; }
        void setPosition(const Vec3& position) { m_position = position; }

        Vec3 getVelocity() const { return m_velocity; }
        void setVelocity(const Vec3& velocity) { m_velocity = velocity; }

        float getMass() const { return m_mass; }
        void setMass(float mass);

        float getGravityScale() const { return m_gravityScale; }
        void setGravityScale(float scale);

        const BoxCollider& getCollider() const { return m_collider; }
        void setCollider(const BoxCollider& collider) {
            m_collider = collider;
        }

        AABB getBounds() const;
        bool isGrounded() const { return m_grounded; }

        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }

        void setCollisionEnabled(bool enabled) {
            m_collisionEnabled = enabled;
        }
        bool isCollisionEnabled() const { return m_collisionEnabled; }

        void setTrigger(bool trigger) { m_trigger = trigger; }
        bool isTrigger() const { return m_trigger; }

    private:
        friend class PhysicsWorld;

        PhysicsBody(
            PhysicsBodyId id,
            PhysicsBodyType type,
            const Vec3& position,
            const BoxCollider& collider);

        float inverseMass() const;

        PhysicsBodyId m_id = 0;
        PhysicsBodyType m_type = PhysicsBodyType::Static;
        Vec3 m_position{};
        Vec3 m_velocity{};
        BoxCollider m_collider;
        float m_mass = 1.0f;
        float m_gravityScale = 1.0f;
        bool m_grounded = false;
        bool m_enabled = true;
        bool m_collisionEnabled = true;
        bool m_trigger = false;
    };

    struct PhysicsCollision {
        PhysicsBodyId firstBody = 0;
        PhysicsBodyId secondBody = 0;
        // Points from the second body towards the first body.
        Vec3 normal{};
        float penetration = 0.0f;
        bool trigger = false;
    };

    class PhysicsWorld {
    public:
        PhysicsBody& createBody(
            PhysicsBodyType type,
            const Vec3& position,
            const BoxCollider& collider = {},
            PhysicsBodyId requestedId = 0);

        PhysicsBody* findBody(PhysicsBodyId id);
        const PhysicsBody* findBody(PhysicsBodyId id) const;
        bool removeBody(PhysicsBodyId id);
        void clear();

        void setGravity(const Vec3& gravity) { m_gravity = gravity; }
        Vec3 getGravity() const { return m_gravity; }

        void setMaximumStep(float seconds);
        float getMaximumStep() const { return m_maximumStep; }

        void setSolverIterations(int iterations);
        int getSolverIterations() const { return m_solverIterations; }

        // Adds a triangle-sampled heightfield beneath dynamic AABB bodies.
        // Passing nullptr removes terrain collision.
        void setHeightMapCollider(
            std::shared_ptr<const HeightMap> heightMap) {
            m_heightMapCollider = std::move(heightMap);
        }
        std::shared_ptr<const HeightMap> getHeightMapCollider() const {
            return m_heightMapCollider;
        }

        void step(float deltaTime);

        const std::vector<std::unique_ptr<PhysicsBody>>& getBodies() const {
            return m_bodies;
        }
        const std::vector<PhysicsCollision>& getCollisions() const {
            return m_collisions;
        }

    private:
        void integrate(float deltaTime);
        void solveCollisions();
        void solveHeightMapCollisions();
        void recordCollision(
            const PhysicsBody& first,
            const PhysicsBody& second,
            const Vec3& normal,
            float penetration,
            bool trigger);
        void recordHeightMapCollision(
            const PhysicsBody& body,
            const Vec3& normal,
            float penetration);
        PhysicsBodyId allocateId();

        Vec3 m_gravity{ 0.0f, -18.0f, 0.0f };
        std::vector<std::unique_ptr<PhysicsBody>> m_bodies;
        std::vector<PhysicsCollision> m_collisions;
        std::shared_ptr<const HeightMap> m_heightMapCollider;
        PhysicsBodyId m_nextId = 1;
        float m_maximumStep = 1.0f / 120.0f;
        int m_solverIterations = 4;
    };

} // namespace WidgeCraft
