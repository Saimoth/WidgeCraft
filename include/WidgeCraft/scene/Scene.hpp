#pragma once

namespace WidgeCraft {

    class WidgeCraft;

    class Scene {
    public:
        virtual ~Scene() = default;

        virtual void onAttach(WidgeCraft& app) { (void)app; }
        virtual void onDetach(WidgeCraft& app) { (void)app; }
        virtual void onUpdate(WidgeCraft& app, float deltaTime) {
            (void)app;
            (void)deltaTime;
        }
        virtual void onRender(WidgeCraft& app) { (void)app; }
    };

} // namespace WidgeCraft
