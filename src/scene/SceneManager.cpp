#include "WidgeCraft/scene/SceneManager.hpp"

#include "WidgeCraft/WidgeCraft.hpp"

#include <utility>

namespace WidgeCraft {

    SceneManager::SceneManager(WidgeCraft& app)
        : m_app(app) {
    }

    Scene& SceneManager::add(
        std::string name,
        std::unique_ptr<Scene> scene) {

        if (name.empty()) {
            throw std::invalid_argument("Scene names cannot be empty");
        }
        if (!scene) {
            throw std::invalid_argument("Cannot add a null scene");
        }
        if (contains(name)) {
            throw std::invalid_argument(
                "A scene named '" + name + "' is already registered");
        }

        Scene& reference = *scene;
        m_scenes.emplace(std::move(name), std::move(scene));
        return reference;
    }

    Scene* SceneManager::find(const std::string& name) {
        const auto iterator = m_scenes.find(name);
        return iterator != m_scenes.end() ? iterator->second.get() : nullptr;
    }

    const Scene* SceneManager::find(const std::string& name) const {
        const auto iterator = m_scenes.find(name);
        return iterator != m_scenes.end() ? iterator->second.get() : nullptr;
    }

    bool SceneManager::contains(const std::string& name) const {
        return find(name) != nullptr;
    }

    bool SceneManager::remove(const std::string& name) {
        Scene* scene = find(name);
        if (!scene || scene == m_active) {
            return false;
        }
        if (m_pendingName && *m_pendingName == name) {
            m_pendingName.reset();
        }
        return m_scenes.erase(name) > 0U;
    }

    bool SceneManager::activate(const std::string& name) {
        Scene* scene = find(name);
        if (!scene) {
            return false;
        }
        if (m_dispatching) {
            return request(name);
        }
        activateNow(scene, name);
        return true;
    }

    void SceneManager::clearActive() {
        if (m_dispatching) {
            requestClear();
            return;
        }
        activateNow(nullptr, {});
    }

    bool SceneManager::request(const std::string& name) {
        if (!contains(name)) {
            return false;
        }
        m_pendingName = name;
        return true;
    }

    void SceneManager::requestClear() {
        m_pendingName = std::string{};
    }

    void SceneManager::setTransient(std::unique_ptr<Scene> scene) {
        if (m_dispatching) {
            throw std::logic_error(
                "setScene cannot replace a scene during its callback; "
                "register it and request the named scene instead");
        }

        if (m_active) {
            m_active->onDetach(m_app);
        }
        m_active = nullptr;
        m_activeName.clear();
        m_transient = std::move(scene);
        m_active = m_transient.get();
        if (m_active) {
            m_active->onAttach(m_app);
        }
    }

    void SceneManager::applyPending() {
        if (!m_pendingName) {
            return;
        }

        std::string name = std::move(*m_pendingName);
        m_pendingName.reset();
        if (name.empty()) {
            activateNow(nullptr, {});
            return;
        }

        if (Scene* scene = find(name)) {
            activateNow(scene, std::move(name));
        }
    }

    void SceneManager::update(float deltaTime) {
        if (!m_active) {
            return;
        }
        m_dispatching = true;
        try {
            m_active->onUpdate(m_app, deltaTime);
        } catch (...) {
            m_dispatching = false;
            throw;
        }
        m_dispatching = false;
    }

    void SceneManager::render() {
        if (!m_active) {
            return;
        }
        m_dispatching = true;
        try {
            m_active->onRender(m_app);
        } catch (...) {
            m_dispatching = false;
            throw;
        }
        m_dispatching = false;
    }

    void SceneManager::shutdown() {
        m_pendingName.reset();
        if (m_active) {
            m_active->onDetach(m_app);
        }
        m_active = nullptr;
        m_activeName.clear();
        m_transient.reset();
        m_scenes.clear();
    }

    void SceneManager::activateNow(Scene* scene, std::string name) {
        if (scene == m_active) {
            m_activeName = std::move(name);
            return;
        }

        if (m_active) {
            m_active->onDetach(m_app);
        }

        m_active = scene;
        m_activeName = std::move(name);
        if (m_active != m_transient.get()) {
            m_transient.reset();
        }

        if (m_active) {
            m_active->onAttach(m_app);
        }
    }

} // namespace WidgeCraft
