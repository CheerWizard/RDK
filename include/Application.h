#pragma once

#include <RenderClient.h>

namespace rdk {

    class Application : WindowListener {

    public:
        Application();
        ~Application();

    public:
        void run();

        void onFrameBufferResized(int width, int height) override;

    private:
        void onCreate();
        void onUpdate();
        void onDestroy();

    private:
        static Application* s_Instance;
        bool m_Running = true;
        Window* m_Window;
        RenderClient* m_RenderClient;
        DrawData m_DrawData;
    };

}