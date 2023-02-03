#pragma once

#include <Window.h>
#include <Debugger.h>
#include <Device.h>
#include <CommandPool.h>

#include <glm/glm.hpp>

#include <memory>

namespace rdk {

    struct Vertex {
        glm::vec2 position;
        glm::vec3 color;
    };

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
        void drawFrame(const DrawData& drawData, u32 instanceCount);
        void onFrameBufferResized(int width, int height);

        void initialize();

        void uploadDrawData(const DrawData& drawData);

        void createVBO(const VertexFormat& vertexFormat, u32 vertexCount = 100);
        void addShader(const std::string& vertFilepath, const std::string& fragFilepath);

    private:
        void createSurface();
        void destroySurface();

    private:
        VkInstance m_Handle;
        Window* m_Window;
        AppInfo m_AppInfo;
        Debugger m_Debugger;
        std::vector<ExtensionProps> m_ExtensionProps;
        VkSurfaceKHR m_Surface;
        Device m_Device;
        // commands and pipeline
        CommandPool m_CommandPool;
        // buffer objects
        VertexBuffer m_Vbo;
        // shaders
        std::shared_ptr<std::vector<Shader>> m_Shaders;
    };

}