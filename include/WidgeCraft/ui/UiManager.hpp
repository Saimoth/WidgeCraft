#pragma once

#include "WidgeCraft/ui/UiScreen.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace WidgeCraft {

    class WidgeCraft;

    class UiManager {
    public:
        explicit UiManager(WidgeCraft& app);
        ~UiManager() = default;

        UiManager(const UiManager&) = delete;
        UiManager& operator=(const UiManager&) = delete;

        UiScreen& add(std::string name, std::unique_ptr<UiScreen> screen);

        template <typename T, typename... Args>
        T& emplace(std::string name, Args&&... args);

        UiScreen* find(const std::string& name);
        const UiScreen* find(const std::string& name) const;
        bool contains(const std::string& name) const;
        bool remove(const std::string& name);

        // activate() is immediate outside a UI callback. During update or
        // rendering it is automatically deferred to the next frame boundary.
        bool activate(const std::string& name);
        void clearActive();
        bool request(const std::string& name);
        void requestClear();
        bool hasPendingChange() const { return m_pendingName.has_value(); }

        UiScreen* getActive() { return m_active; }
        const UiScreen* getActive() const { return m_active; }
        const std::string& getActiveName() const { return m_activeName; }

    private:
        friend class WidgeCraft;

        void applyPending();
        void update(float deltaTime);
        void renderSceneViews();
        void renderUi();
        void shutdown();
        void activateNow(UiScreen* screen, std::string name);
        void synchronizeRoot(UiScreen& screen);

        WidgeCraft& m_app;
        std::unordered_map<std::string, std::unique_ptr<UiScreen>> m_screens;
        UiScreen* m_active = nullptr;
        std::string m_activeName;
        std::optional<std::string> m_pendingName;
        bool m_dispatching = false;
    };

    template <typename T, typename... Args>
    T& UiManager::emplace(std::string name, Args&&... args) {
        static_assert(
            std::is_base_of_v<UiScreen, T>,
            "UiManager::emplace requires a UiScreen-derived type");
        auto screen = std::make_unique<T>(std::forward<Args>(args)...);
        T& reference = *screen;
        add(std::move(name), std::move(screen));
        return reference;
    }

} // namespace WidgeCraft
