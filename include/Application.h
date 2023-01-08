#pragma once

namespace rect {

    class Application final {

    public:
        Application();
        ~Application();

    public:
        void run();

    private:
        void onCreate();
        void onUpdate();
        void onDestroy();

    private:
        static Application* s_Instance;
        bool m_Running = true;
    };

}