#pragma once

#include <Window.h>
#include <Debugger.h>
#include <Device.h>
#include <CommandPool.h>
#include <Image.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <memory>
#include <chrono>

namespace rdk {

    struct Vertex final {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
    };

    struct RectVertexData final {
        // rect 1
        Vertex v0 = {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
        Vertex v1 = {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}};
        Vertex v2 = {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}};
        Vertex v3 = {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}};
        // rect 2
        Vertex v4 = {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
        Vertex v5 = {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}};
        Vertex v6 = {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}};
        Vertex v7 = {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}};
    };

    struct Rect final {
        static const u32 INDEX_COUNT = 12;

        RectVertexData vertexData;
        u32 indices[INDEX_COUNT] = {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4,
        };

        static u32 vertex_size() { return sizeof(RectVertexData); }
        static u32 index_size() { return sizeof(u32) * INDEX_COUNT; }
        void* data() { return &vertexData.v0; }
    };

    // this struct should be aligned with shader uniform buffer layout
    struct MVP final {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct AppInfo final {
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

        void beginFrame();
        void endFrame();

        void drawVertices(u32 vertexCount, u32 instanceCount);
        void drawIndices(u32 indexCount, u32 instanceCount);

        void onFrameBufferResized(int width, int height);

        void initialize();

        void createVertexBuffer(const VertexData& vertexData);
        void createIndexBuffer(const IndexData& indexData);
        void createUniformBuffers(VkDeviceSize size);

        void addShader(const std::string& vertFilepath, const std::string& fragFilepath);

        void createRect();

        MVP createMVP(float aspect);
        void updateMVP(MVP& mvp);

        void createTexture2D(const char* filepath);

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
        Pipeline m_Pipeline;
        SwapChain m_SwapChain;
        // descriptors
        DescriptorPool m_DescriptorPool;
        // buffer objects
        Buffer m_VertexBuffer;
        Buffer m_IndexBuffer;
        std::vector<Buffer> m_UniformBuffers;
        std::vector<void*> m_UniformBufferBlocks;
        // shaders
        std::shared_ptr<std::vector<Shader>> m_Shaders;
        // timing
        float m_DeltaTime = 0;
        std::chrono::time_point<std::chrono::steady_clock> m_BeginTime;
        // images
        std::vector<Image> m_Images;
        std::vector<ImageView> m_ImageViews;
        std::vector<ImageSampler> m_ImageSamplers;
    };

}