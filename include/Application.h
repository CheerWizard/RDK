#pragma once

#include <cstdint>
#include <vector>

typedef uint32_t u32;

namespace rect {

    // ---------------- Window code ------------------------- //

    enum Resizable : unsigned char {
        R_FALSE = 0, R_TRUE = 1
    };

    class Window final {

    public:
        Window(const char* title, int width, int height);
        ~Window();

    public:
        void update();
        void setResizable(Resizable resizable);
        inline bool shouldClose();
        inline const std::vector<const char*>& getExtensions();
        void addExtension(const char* && extension);

    private:
        void* m_Handle;
        const char* m_Title;
        int m_Width, m_Height;
        std::vector<const char*> m_Extensions;
    };

    // ---------------- Graphics code ----------------------- //

    struct AppInfo final {
        int type;
        const void* next = nullptr;
        const char* appName;
        u32 appVersion;
        const char* engineName;
        u32 engineVersion;
        u32 apiVersion;
    };

    struct ExtensionProps final {
        const char* name;
        u32 version;

        ExtensionProps(const char* in_name, u32 in_version) : name(in_name), version(in_version) {}
    };

    class GraphicsInstance final {

    public:
        explicit GraphicsInstance(const AppInfo& appInfo, Window* window);
        ~GraphicsInstance();

    public:
        void printExtensions();

    private:
        void* m_Instance;
        AppInfo m_AppInfo;
        std::vector<ExtensionProps> m_ExtensionProps;
    };

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
        Window* m_Window;
        GraphicsInstance* m_GraphicsInstance;
    };

}