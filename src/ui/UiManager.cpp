#include "WidgeCraft/ui/UiManager.hpp"

#include "WidgeCraft/WidgeCraft.hpp"

#include <utility>

namespace WidgeCraft {

    UiManager::UiManager(WidgeCraft& app)
        : m_app(app) {
    }

    UiScreen& UiManager::add(
        std::string name,
        std::unique_ptr<UiScreen> screen) {

        if (name.empty()) {
            throw std::invalid_argument("UI registration names cannot be empty");
        }
        if (!screen) {
            throw std::invalid_argument("Cannot add a null UI screen");
        }
        if (contains(name)) {
            throw std::invalid_argument(
                "A UI screen named '" + name + "' is already registered");
        }

        UiScreen& reference = *screen;
        m_screens.emplace(std::move(name), std::move(screen));
        return reference;
    }

    UiScreen* UiManager::find(const std::string& name) {
        const auto iterator = m_screens.find(name);
        return iterator != m_screens.end() ? iterator->second.get() : nullptr;
    }

    const UiScreen* UiManager::find(const std::string& name) const {
        const auto iterator = m_screens.find(name);
        return iterator != m_screens.end() ? iterator->second.get() : nullptr;
    }

    bool UiManager::contains(const std::string& name) const {
        return find(name) != nullptr;
    }

    bool UiManager::remove(const std::string& name) {
        UiScreen* screen = find(name);
        if (!screen || screen == m_active) {
            return false;
        }
        if (m_pendingName && *m_pendingName == name) {
            m_pendingName.reset();
        }
        return m_screens.erase(name) > 0U;
    }

    bool UiManager::activate(const std::string& name) {
        UiScreen* screen = find(name);
        if (!screen) {
            return false;
        }
        if (m_dispatching) {
            return request(name);
        }
        activateNow(screen, name);
        return true;
    }

    void UiManager::clearActive() {
        if (m_dispatching) {
            requestClear();
            return;
        }
        activateNow(nullptr, {});
    }

    bool UiManager::request(const std::string& name) {
        if (!contains(name)) {
            return false;
        }
        m_pendingName = name;
        return true;
    }

    void UiManager::requestClear() {
        m_pendingName = std::string{};
    }

    void UiManager::applyPending() {
        if (!m_pendingName) {
            return;
        }

        std::string name = std::move(*m_pendingName);
        m_pendingName.reset();
        if (name.empty()) {
            activateNow(nullptr, {});
            return;
        }

        if (UiScreen* screen = find(name)) {
            activateNow(screen, std::move(name));
        }
    }

    void UiManager::update(float deltaTime) {
        if (!m_active) {
            return;
        }
        synchronizeRoot(*m_active);
        m_dispatching = true;
        try {
            m_active->onUpdate(m_app, deltaTime);
        } catch (...) {
            m_dispatching = false;
            throw;
        }
        m_dispatching = false;
    }

    void UiManager::renderSceneViews() {
        if (!m_active) {
            return;
        }
        synchronizeRoot(*m_active);
        m_dispatching = true;
        try {
            m_active->onRender(m_app);
            m_active->renderSceneViews(m_app);
        } catch (...) {
            m_dispatching = false;
            throw;
        }
        m_dispatching = false;
    }

    void UiManager::renderUi() {
        if (!m_active) {
            return;
        }
        synchronizeRoot(*m_active);
        m_dispatching = true;
        try {
            m_active->getRootFrame().render(
                m_app.getTextRenderer(),
                m_app.getShapes2D(),
                m_app.getInput());
        } catch (...) {
            m_dispatching = false;
            throw;
        }
        m_dispatching = false;
    }

    void UiManager::shutdown() {
        m_pendingName.reset();
        if (m_active) {
            m_active->onDetach(m_app);
        }
        m_active = nullptr;
        m_activeName.clear();
        m_screens.clear();
    }

    void UiManager::activateNow(UiScreen* screen, std::string name) {
        if (screen == m_active) {
            m_activeName = std::move(name);
            return;
        }

        if (m_active) {
            m_active->onDetach(m_app);
        }
        m_active = screen;
        m_activeName = std::move(name);
        if (m_active) {
            synchronizeRoot(*m_active);
            m_active->onAttach(m_app);
        }
    }

    void UiManager::synchronizeRoot(UiScreen& screen) {
        screen.setScreenSize(
            static_cast<float>(m_app.getWindow().getWidth()),
            static_cast<float>(m_app.getWindow().getHeight()));
    }

} // namespace WidgeCraft
