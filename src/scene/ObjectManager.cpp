#include "WidgeCraft/scene/ObjectManager.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace WidgeCraft {

    namespace {
        bool overlaps(const AABB& first, const AABB& second) {
            return first.minimum.x <= second.maximum.x
                && first.maximum.x >= second.minimum.x
                && first.minimum.y <= second.maximum.y
                && first.maximum.y >= second.minimum.y
                && first.minimum.z <= second.maximum.z
                && first.maximum.z >= second.minimum.z;
        }

        Vec3 sanitizeScale(const Vec3& scale) {
            const auto component = [](float value) {
                if (std::abs(value) >= 0.0001f) {
                    return value;
                }
                return value < 0.0f ? -0.0001f : 0.0001f;
            };
            return {
                component(scale.x),
                component(scale.y),
                component(scale.z)
            };
        }
    } // namespace

    SceneObject::SceneObject(
        ObjectId id,
        std::string name,
        std::shared_ptr<const Mesh3D> mesh,
        const Transform3D& transform)
        : m_id(id)
        , m_name(std::move(name))
        , m_mesh(std::move(mesh))
        , m_transform(transform) {
        m_transform.scale = sanitizeScale(m_transform.scale);
    }

    void SceneObject::setTransform(const Transform3D& transform) {
        m_transform = transform;
        m_transform.scale = sanitizeScale(m_transform.scale);
        m_transformDirty = true;
    }

    void SceneObject::setPosition(const Vec3& position) {
        m_transform.position = position;
        m_transformDirty = true;
    }

    void SceneObject::setRotation(const Vec3& rotation) {
        m_transform.rotation = rotation;
        m_transformDirty = true;
    }

    void SceneObject::setScale(const Vec3& scale) {
        m_transform.scale = sanitizeScale(scale);
        m_transformDirty = true;
    }

    void SceneObject::setMesh(std::shared_ptr<const Mesh3D> mesh) {
        if (!mesh || !mesh->isValid()) {
            throw std::invalid_argument(
                "Scene objects require a valid triangle mesh");
        }
        m_mesh = std::move(mesh);
        m_transformDirty = true;
    }

    AABB SceneObject::getWorldBounds() const {
        if (!m_mesh || m_mesh->vertices.empty()) {
            return { m_transform.position, m_transform.position };
        }

        const float maximum = std::numeric_limits<float>::max();
        Vec3 minimum{ maximum, maximum, maximum };
        Vec3 upper{ -maximum, -maximum, -maximum };
        for (const Vec3& vertex : m_mesh->vertices) {
            const Vec3 world = m_transform.transformPoint(vertex);
            minimum.x = std::min(minimum.x, world.x);
            minimum.y = std::min(minimum.y, world.y);
            minimum.z = std::min(minimum.z, world.z);
            upper.x = std::max(upper.x, world.x);
            upper.y = std::max(upper.y, world.y);
            upper.z = std::max(upper.z, world.z);
        }
        return { minimum, upper };
    }

    SceneObject& ObjectManager::createMeshObject(
        std::string name,
        std::shared_ptr<const Mesh3D> mesh,
        const Transform3D& transform,
        ObjectId requestedId) {

        if (!mesh || !mesh->isValid()) {
            throw std::invalid_argument(
                "ObjectManager requires a valid triangle mesh");
        }
        const ObjectId id = requestedId == 0
            ? allocateId()
            : requestedId;
        if (find(id)) {
            throw std::invalid_argument(
                "An object with the requested ID already exists");
        }
        m_nextId = std::max(m_nextId, id + 1U);

        auto object = std::unique_ptr<SceneObject>(new SceneObject(
            id,
            std::move(name),
            std::move(mesh),
            transform));
        SceneObject& reference = *object;
        m_objects.emplace_back(std::move(object));
        return reference;
    }

    SceneObject& ObjectManager::createBoxObject(
        std::string name,
        const Vec3& halfExtents,
        const Transform3D& transform,
        ObjectId requestedId) {
        return createMeshObject(
            std::move(name),
            std::make_shared<Mesh3D>(makeBoxMesh(halfExtents)),
            transform,
            requestedId);
    }

    SceneObject* ObjectManager::find(ObjectId id) {
        const auto iterator = std::find_if(
            m_objects.begin(),
            m_objects.end(),
            [id](const auto& object) {
                return object && object->getId() == id;
            });
        return iterator != m_objects.end() ? iterator->get() : nullptr;
    }

    const SceneObject* ObjectManager::find(ObjectId id) const {
        const auto iterator = std::find_if(
            m_objects.begin(),
            m_objects.end(),
            [id](const auto& object) {
                return object && object->getId() == id;
            });
        return iterator != m_objects.end() ? iterator->get() : nullptr;
    }

    bool ObjectManager::remove(ObjectId id) {
        const auto iterator = std::find_if(
            m_objects.begin(),
            m_objects.end(),
            [id](const auto& object) {
                return object && object->getId() == id;
            });
        if (iterator == m_objects.end()) {
            return false;
        }
        detachPhysics(**iterator);
        m_objects.erase(iterator);
        return true;
    }

    void ObjectManager::clear() {
        if (m_physicsWorld) {
            for (auto& object : m_objects) {
                if (object) {
                    detachPhysics(*object);
                }
            }
        }
        m_objects.clear();
        m_nextId = 1;
    }

    void ObjectManager::setDrawDistance(float distance) {
        m_drawDistance = std::max(distance, 0.001f);
    }

    ObjectRenderStats ObjectManager::render(
        Shapes3D& shapes,
        const Vec3& viewPosition,
        const ModelViewport* viewport,
        TextRenderer* textRenderer) const {

        const auto start = std::chrono::steady_clock::now();
        ObjectRenderStats stats;
        const float drawDistanceSquared = m_drawDistance * m_drawDistance;

        for (const auto& objectPointer : m_objects) {
            const SceneObject* object = objectPointer.get();
            if (!object) {
                continue;
            }
            ++stats.considered;

            const ObjectAttributes& attributes = object->getAttributes();
            if (!attributes.enabled || !attributes.rendered) {
                ++stats.hidden;
                continue;
            }

            const Vec3 delta = object->getTransform().position - viewPosition;
            if (lengthSquared(delta) > drawDistanceSquared) {
                ++stats.distanceCulled;
                continue;
            }

            const std::shared_ptr<const Mesh3D> mesh = object->getMesh();
            if (!mesh || !mesh->isValid()) {
                ++stats.hidden;
                continue;
            }

            shapes.drawMesh(
                *mesh,
                object->getTransform(),
                object->getStyle());
            ++stats.rendered;
            stats.trianglesQueued += mesh->getTriangleCount();

            if (attributes.labelVisible && viewport && textRenderer) {
                const AABB bounds = object->getWorldBounds();
                if (viewport->drawLabel3D(
                        *textRenderer,
                        "ID " + std::to_string(object->getId()),
                        {
                            (bounds.minimum.x + bounds.maximum.x) * 0.5f,
                            bounds.maximum.y + 0.25f,
                            (bounds.minimum.z + bounds.maximum.z) * 0.5f
                        },
                        13.0f,
                        { 0.82f, 0.92f, 1.0f, 0.92f })) {
                    ++stats.labelsQueued;
                }
            }
        }

        stats.vertexBytesQueued = shapes.getQueuedVertexBytes();
        const auto end = std::chrono::steady_clock::now();
        stats.cpuMilliseconds = std::chrono::duration<
            double,
            std::milli>(end - start).count();
        return stats;
    }

    bool ObjectManager::raycastInteractable(
        const Ray& ray,
        ObjectRayHit& result,
        float maximumDistance) const {

        float nearestDistance = maximumDistance > 0.0f
            ? maximumDistance
            : std::numeric_limits<float>::max();
        bool found = false;
        for (const auto& objectPointer : m_objects) {
            const SceneObject* object = objectPointer.get();
            if (!object) {
                continue;
            }
            const ObjectAttributes& attributes = object->getAttributes();
            if (!attributes.enabled
                || !attributes.interactable
                || !attributes.selectable) {
                continue;
            }

            RayHit hit;
            if (raycast(ray, object->getWorldBounds(), hit)
                && hit.distance < nearestDistance) {
                nearestDistance = hit.distance;
                result = { object->getId(), hit };
                found = true;
            }
        }
        return found;
    }

    std::vector<ObjectId> ObjectManager::queryTriggers(
        const AABB& bounds) const {

        std::vector<ObjectId> result;
        for (const auto& objectPointer : m_objects) {
            const SceneObject* object = objectPointer.get();
            if (object
                && object->getAttributes().enabled
                && object->getAttributes().trigger
                && overlaps(bounds, object->getWorldBounds())) {
                result.push_back(object->getId());
            }
        }
        return result;
    }

    void ObjectManager::setPhysicsWorld(PhysicsWorld* physicsWorld) {
        if (m_physicsWorld == physicsWorld) {
            return;
        }
        if (m_physicsWorld) {
            for (auto& object : m_objects) {
                if (object) {
                    detachPhysics(*object);
                }
            }
        }
        m_physicsWorld = physicsWorld;
    }

    void ObjectManager::preparePhysics() {
        if (!m_physicsWorld) {
            return;
        }

        for (auto& objectPointer : m_objects) {
            SceneObject* object = objectPointer.get();
            if (!object) {
                continue;
            }
            const ObjectAttributes& attributes = object->getAttributes();
            const bool needsBody = attributes.enabled
                && (attributes.collidable
                    || attributes.trigger
                    || attributes.gravity);
            if (!needsBody) {
                detachPhysics(*object);
                continue;
            }

            PhysicsBody* body = getPhysicsBody(*object);
            bool created = false;
            if (!body) {
                body = &m_physicsWorld->createBody(
                    attributes.fixed
                        ? PhysicsBodyType::Static
                        : PhysicsBodyType::Dynamic,
                    object->getTransform().position);
                object->m_physicsBodyId = body->getId();
                created = true;
            }

            body->setType(
                attributes.fixed
                    ? PhysicsBodyType::Static
                    : PhysicsBodyType::Dynamic);
            body->setEnabled(attributes.enabled);
            body->setCollisionEnabled(
                attributes.collidable || attributes.trigger);
            body->setTrigger(attributes.trigger);
            body->setGravityScale(attributes.gravity ? 1.0f : 0.0f);

            const AABB bounds = object->getWorldBounds();
            const Vec3 center = (bounds.minimum + bounds.maximum) * 0.5f;
            const Vec3 half = (bounds.maximum - bounds.minimum) * 0.5f;
            body->setCollider(BoxCollider(
                half,
                center - object->getTransform().position));

            if (created || attributes.fixed || object->m_transformDirty) {
                body->setPosition(object->getTransform().position);
                object->m_transformDirty = false;
            }
        }
    }

    void ObjectManager::syncFromPhysics() {
        if (!m_physicsWorld) {
            return;
        }
        for (auto& objectPointer : m_objects) {
            SceneObject* object = objectPointer.get();
            if (!object || object->getAttributes().fixed) {
                continue;
            }
            const PhysicsBody* body = getPhysicsBody(*object);
            if (body) {
                object->m_transform.position = body->getPosition();
                object->m_transformDirty = false;
            }
        }
    }

    PhysicsBody* ObjectManager::getPhysicsBody(SceneObject& object) {
        return m_physicsWorld && object.m_physicsBodyId != 0
            ? m_physicsWorld->findBody(object.m_physicsBodyId)
            : nullptr;
    }

    const PhysicsBody* ObjectManager::getPhysicsBody(
        const SceneObject& object) const {
        return m_physicsWorld && object.m_physicsBodyId != 0
            ? m_physicsWorld->findBody(object.m_physicsBodyId)
            : nullptr;
    }

    std::vector<ObjectId> ObjectManager::getTriggeredObjects(
        PhysicsBodyId otherBodyId) const {

        std::vector<ObjectId> result;
        if (!m_physicsWorld) {
            return result;
        }
        for (const PhysicsCollision& collision
             : m_physicsWorld->getCollisions()) {
            if (!collision.trigger) {
                continue;
            }

            PhysicsBodyId triggerBody = 0;
            if (collision.firstBody == otherBodyId) {
                triggerBody = collision.secondBody;
            } else if (collision.secondBody == otherBodyId) {
                triggerBody = collision.firstBody;
            } else {
                continue;
            }

            const auto iterator = std::find_if(
                m_objects.begin(),
                m_objects.end(),
                [triggerBody](const auto& object) {
                    return object
                        && object->m_physicsBodyId == triggerBody
                        && object->getAttributes().trigger;
                });
            if (iterator != m_objects.end()) {
                result.push_back((*iterator)->getId());
            }
        }
        return result;
    }

    std::size_t ObjectManager::getApproximateMemoryUsageBytes() const {
        std::size_t bytes = m_objects.capacity()
            * sizeof(std::unique_ptr<SceneObject>);
        std::unordered_set<const Mesh3D*> meshes;
        for (const auto& object : m_objects) {
            if (!object) {
                continue;
            }
            bytes += sizeof(SceneObject) + object->m_name.capacity();
            if (object->m_mesh
                && meshes.insert(object->m_mesh.get()).second) {
                bytes += sizeof(Mesh3D)
                    + object->m_mesh->getMemoryUsageBytes();
            }
        }
        return bytes;
    }

    ObjectId ObjectManager::allocateId() {
        while (find(m_nextId)) {
            ++m_nextId;
        }
        return m_nextId++;
    }

    void ObjectManager::detachPhysics(SceneObject& object) {
        if (m_physicsWorld && object.m_physicsBodyId != 0) {
            m_physicsWorld->removeBody(object.m_physicsBodyId);
        }
        object.m_physicsBodyId = 0;
    }

} // namespace WidgeCraft
