#include "WidgeCraft/physics/PhysicsWorld.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace WidgeCraft {

    namespace {
        bool calculateOverlap(
            const PhysicsBody& first,
            const PhysicsBody& second,
            Vec3& normal,
            float& penetration) {

            const AABB firstBounds = first.getBounds();
            const AABB secondBounds = second.getBounds();
            const float overlapX = std::min(
                firstBounds.maximum.x,
                secondBounds.maximum.x)
                - std::max(
                    firstBounds.minimum.x,
                    secondBounds.minimum.x);
            const float overlapY = std::min(
                firstBounds.maximum.y,
                secondBounds.maximum.y)
                - std::max(
                    firstBounds.minimum.y,
                    secondBounds.minimum.y);
            const float overlapZ = std::min(
                firstBounds.maximum.z,
                secondBounds.maximum.z)
                - std::max(
                    firstBounds.minimum.z,
                    secondBounds.minimum.z);

            if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f) {
                return false;
            }

            const Vec3 firstCenter =
                (firstBounds.minimum + firstBounds.maximum) * 0.5f;
            const Vec3 secondCenter =
                (secondBounds.minimum + secondBounds.maximum) * 0.5f;
            const Vec3 centerDelta = firstCenter - secondCenter;

            penetration = overlapX;
            normal = { centerDelta.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f };
            if (overlapY < penetration) {
                penetration = overlapY;
                normal = {
                    0.0f,
                    centerDelta.y < 0.0f ? -1.0f : 1.0f,
                    0.0f
                };
            }
            if (overlapZ < penetration) {
                penetration = overlapZ;
                normal = {
                    0.0f,
                    0.0f,
                    centerDelta.z < 0.0f ? -1.0f : 1.0f
                };
            }
            return true;
        }
    } // namespace

    PhysicsBody::PhysicsBody(
        PhysicsBodyId id,
        PhysicsBodyType type,
        const Vec3& position,
        const BoxCollider& collider)
        : m_id(id)
        , m_type(type)
        , m_position(position)
        , m_collider(collider) {
    }

    void PhysicsBody::setType(PhysicsBodyType type) {
        m_type = type;
        m_grounded = false;
        if (isStatic()) {
            m_velocity = {};
        }
    }

    void PhysicsBody::setMass(float mass) {
        m_mass = std::max(mass, 0.001f);
    }

    void PhysicsBody::setGravityScale(float scale) {
        m_gravityScale = std::max(scale, 0.0f);
    }

    AABB PhysicsBody::getBounds() const {
        const Vec3 center = m_position + m_collider.offset;
        const Vec3 halfExtents{
            std::max(std::abs(m_collider.halfExtents.x), 0.001f),
            std::max(std::abs(m_collider.halfExtents.y), 0.001f),
            std::max(std::abs(m_collider.halfExtents.z), 0.001f)
        };
        return {
            center - halfExtents,
            center + halfExtents
        };
    }

    float PhysicsBody::inverseMass() const {
        return isDynamic() && m_enabled ? 1.0f / m_mass : 0.0f;
    }

    PhysicsBody& PhysicsWorld::createBody(
        PhysicsBodyType type,
        const Vec3& position,
        const BoxCollider& collider,
        PhysicsBodyId requestedId) {

        const PhysicsBodyId id = requestedId == 0
            ? allocateId()
            : requestedId;
        if (findBody(id)) {
            throw std::invalid_argument(
                "A physics body with the requested ID already exists");
        }

        m_nextId = std::max(m_nextId, id + 1);
        auto body = std::unique_ptr<PhysicsBody>(
            new PhysicsBody(id, type, position, collider));
        PhysicsBody& reference = *body;
        m_bodies.emplace_back(std::move(body));
        return reference;
    }

    PhysicsBody* PhysicsWorld::findBody(PhysicsBodyId id) {
        const auto iterator = std::find_if(
            m_bodies.begin(),
            m_bodies.end(),
            [id](const auto& body) {
                return body && body->getId() == id;
            });
        return iterator != m_bodies.end() ? iterator->get() : nullptr;
    }

    const PhysicsBody* PhysicsWorld::findBody(PhysicsBodyId id) const {
        const auto iterator = std::find_if(
            m_bodies.begin(),
            m_bodies.end(),
            [id](const auto& body) {
                return body && body->getId() == id;
            });
        return iterator != m_bodies.end() ? iterator->get() : nullptr;
    }

    bool PhysicsWorld::removeBody(PhysicsBodyId id) {
        const auto iterator = std::find_if(
            m_bodies.begin(),
            m_bodies.end(),
            [id](const auto& body) {
                return body && body->getId() == id;
            });
        if (iterator == m_bodies.end()) {
            return false;
        }
        m_bodies.erase(iterator);
        return true;
    }

    void PhysicsWorld::clear() {
        m_bodies.clear();
        m_collisions.clear();
        m_nextId = 1;
    }

    void PhysicsWorld::setMaximumStep(float seconds) {
        m_maximumStep = std::clamp(seconds, 1.0e-4f, 0.1f);
    }

    void PhysicsWorld::setSolverIterations(int iterations) {
        m_solverIterations = std::clamp(iterations, 1, 32);
    }

    void PhysicsWorld::step(float deltaTime) {
        const float frameTime = std::clamp(deltaTime, 0.0f, 0.25f);
        if (frameTime <= 0.0f) {
            return;
        }

        m_collisions.clear();
        for (auto& body : m_bodies) {
            if (body && body->isDynamic()) {
                body->m_grounded = false;
            }
        }

        const int substepCount = std::max(
            1,
            static_cast<int>(std::ceil(frameTime / m_maximumStep)));
        const float substepTime = frameTime
            / static_cast<float>(substepCount);
        for (int substep = 0; substep < substepCount; ++substep) {
            integrate(substepTime);
            for (int iteration = 0;
                 iteration < m_solverIterations;
                 ++iteration) {
                solveCollisions();
            }
        }
    }

    void PhysicsWorld::integrate(float deltaTime) {
        for (auto& body : m_bodies) {
            if (!body || !body->isEnabled() || !body->isDynamic()) {
                continue;
            }
            body->m_velocity +=
                m_gravity * (body->m_gravityScale * deltaTime);
            body->m_position += body->m_velocity * deltaTime;
        }
    }

    void PhysicsWorld::solveCollisions() {
        for (std::size_t firstIndex = 0;
             firstIndex < m_bodies.size();
             ++firstIndex) {
            PhysicsBody* first = m_bodies[firstIndex].get();
            if (!first || !first->isEnabled()) {
                continue;
            }

            for (std::size_t secondIndex = firstIndex + 1;
                 secondIndex < m_bodies.size();
                 ++secondIndex) {
                PhysicsBody* second = m_bodies[secondIndex].get();
                if (!second || !second->isEnabled()) {
                    continue;
                }

                const float firstInverseMass = first->inverseMass();
                const float secondInverseMass = second->inverseMass();
                const float inverseMassSum =
                    firstInverseMass + secondInverseMass;
                if (inverseMassSum <= 0.0f) {
                    continue;
                }

                Vec3 normal{};
                float penetration = 0.0f;
                if (!calculateOverlap(
                        *first,
                        *second,
                        normal,
                        penetration)) {
                    continue;
                }

                const Vec3 correction = normal * penetration;
                first->m_position += correction
                    * (firstInverseMass / inverseMassSum);
                second->m_position -= correction
                    * (secondInverseMass / inverseMassSum);

                const Vec3 relativeVelocity =
                    first->m_velocity - second->m_velocity;
                const float normalVelocity = dot(relativeVelocity, normal);
                if (normalVelocity < 0.0f) {
                    const float impulseMagnitude =
                        -normalVelocity / inverseMassSum;
                    first->m_velocity += normal
                        * (impulseMagnitude * firstInverseMass);
                    second->m_velocity -= normal
                        * (impulseMagnitude * secondInverseMass);
                }

                if (first->isDynamic() && normal.y > 0.5f) {
                    first->m_grounded = true;
                }
                if (second->isDynamic() && normal.y < -0.5f) {
                    second->m_grounded = true;
                }

                recordCollision(
                    *first,
                    *second,
                    normal,
                    penetration);
            }
        }
    }

    void PhysicsWorld::recordCollision(
        const PhysicsBody& first,
        const PhysicsBody& second,
        const Vec3& normal,
        float penetration) {

        const auto iterator = std::find_if(
            m_collisions.begin(),
            m_collisions.end(),
            [&](const PhysicsCollision& collision) {
                return collision.firstBody == first.getId()
                    && collision.secondBody == second.getId();
            });
        if (iterator != m_collisions.end()) {
            if (penetration > iterator->penetration) {
                iterator->normal = normal;
                iterator->penetration = penetration;
            }
            return;
        }
        m_collisions.push_back({
            first.getId(),
            second.getId(),
            normal,
            penetration
        });
    }

    PhysicsBodyId PhysicsWorld::allocateId() {
        while (findBody(m_nextId)) {
            ++m_nextId;
        }
        return m_nextId++;
    }

} // namespace WidgeCraft
