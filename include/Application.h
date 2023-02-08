#pragma once

#include <Renderer.h>

namespace rdk {

    class Application : WindowListener, RenderListener {

    public:
        void run();

        void onFrameBufferResized(int width, int height) override;

    private:
        void onCreate();
        void onUpdate();
        void onDestroy();

        void onRender(float dt) override;
        void onRenderUI(float dt) override;

    private:
        bool m_Running = true;
        Window* m_Window;
        Renderer* m_Renderer;
        MVP m_MVP;
    };

}