#pragma once

#include <Window.h>
#include <Debugger.h>
#include <Device.h>
#include <CommandPool.h>

namespace rdk {

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
        std::string name;
        u32 version;

        ExtensionProps(const char* in_name, u32 in_version) : name(in_name), version(in_version) {}
    };

    class RenderClient final {

    public:
        explicit RenderClient(const AppInfo& appInfo, Window* window);
        ~RenderClient();

    public:
        void printExtensions();
        void drawFrame(u32 vertexCount, u32 instanceCount);
        void onFrameBufferResized(int width, int height);

    private:
        void createSurface();
        void destroySurface();

    private:
        void* m_Handle;
        Window* m_Window;
        AppInfo m_AppInfo;
        Debugger m_Debugger;
        std::vector<ExtensionProps> m_ExtensionProps;
        void* m_Surface;
        Device m_Device;
        // commands and pipeline
        CommandPool m_CommandPool;
    };

}