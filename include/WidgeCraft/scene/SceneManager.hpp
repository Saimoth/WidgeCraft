#pragma once

#include "WidgeCraft/scene/Scene.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace WidgeCraft {

    class WidgeCraft;

    class SceneManager {
    public:
        explicit SceneManager(WidgeCraft& app);
        ~SceneManager() = default;

        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        Scene& add(std::string name, std::unique_ptr<Scene> scene);

        template <typename T, typename... Args>
        T& emplace(std::string name, Args&&... args);

        Scene* find(const std::string& name);
        const Scene* find(const std::string& name) const;
        bool contains(const std::string& name) const;
        bool remove(const std::string& name);

        // activate() is immediate outside a scene callback. During update or
        // render it is automatically deferred to the next frame boundary.
        bool activate(const std::string& name);
        void clearActive();

        // Requests are the safe choice from scenes and UI callbacks. Scene and
        // UI manager requests are both applied before the next update.
        bool request(const std::string& name);
        void requestClear();
        bool hasPendingChange() const { return m_pendingName.has_value(); }

        Scene* getActive() { return m_active; }
        const Scene* getActive() const { return m_active; }
        const std::string& getActiveName() const { return m_activeName; }

    private:
        friend class WidgeCraft;

        void setTransient(std::unique_ptr<Scene> scene);
        void applyPending();
        void update(float deltaTime);
        void render();
        void shutdown();
        void activateNow(Scene* scene, std::string name);

        WidgeCraft& m_app;
        std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
        std::unique_ptr<Scene> m_transient;
        Scene* m_active = nullptr;
        std::string m_activeName;
        std::optional<std::string> m_pendingName;
        bool m_dispatching = false;
    };

    template <typename T, typename... Args>
    T& SceneManager::emplace(std::string name, Args&&... args) {
        static_assert(
            std::is_base_of_v<Scene, T>,
            "SceneManager::emplace requires a Scene-derived type");
        auto scene = std::make_unique<T>(std::forward<Args>(args)...);
        T& reference = *scene;
        add(std::move(name), std::move(scene));
        return reference;
    }

} // namespace WidgeCraft
