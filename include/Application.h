#pragma once

#include <cstdint>
#include <vector>
#include <string>

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
        inline void* getHandle();

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
        std::string name;
        u32 version;

        ExtensionProps(const char* in_name, u32 in_version) : name(in_name), version(in_version) {}
    };

    struct QueueFamilyIndices final {
        static const int NONE_FAMILY = -1;

        int graphicsFamily = NONE_FAMILY;
        int presentationFamily = NONE_FAMILY;

        inline bool completed() const;
    };

    struct Extent2D final {
        u32 width, height;
    };

    struct ShaderStage final {
        int sType;
        const void* next = nullptr;
        u32 flags;
        u32 stage;
        void* module;
        const char* name;
        const void* specInfo = nullptr;

        ShaderStage() = default;
        ShaderStage(void* logicalDevice, const std::vector<char>& code);

        void cleanup(void* logicalDevice);
    };

    class Shader final {

    public:
        Shader() = default;
        Shader(void* logicalDevice, const std::string& vertFilepath, const std::string& fragFilepath);
        ~Shader();

    public:
        inline const ShaderStage& getVertStage() const;
        inline const ShaderStage& getFragStage() const;

    private:
        void cleanup();

    private:
        void* m_LogicalDevice = nullptr;
        ShaderStage m_VertStage;
        ShaderStage m_FragStage;
    };

    class GraphicsInstance final {

    public:
        explicit GraphicsInstance(const AppInfo& appInfo, Window* window);
        ~GraphicsInstance();

    public:
        void printExtensions();

    private:
        bool isLayerValidationSupported();
        void createDebugger();
        void destroyDebugger();
        void createPhysicalDevice();
        bool isDeviceSuitable(void* physicalDevice);
        QueueFamilyIndices findQueueFamilies(void* physicalDevice);
        void createLogicalDevice();
        void destroyLogicalDevice();
        void createSurface();
        void destroySurface();
        bool isDeviceExtensionSupported(void* physicalDevice);
        void createSwapChain();
        void destroySwapChain();
        void querySwapChainImages(u32 imageCount);
        void createImageViews();
        void destroyImageViews();
        void createRenderPass();
        void destroyRenderPass();
        void createPipeline();
        void destroyPipeline();

    private:
        void* m_Instance;
        Window* m_Window;
        AppInfo m_AppInfo;
        std::vector<ExtensionProps> m_ExtensionProps;
        std::vector<const char*> m_ValidationLayers;
        void* m_Debugger;
        void* m_Surface;
        void* m_PhysicalDevice;
        // logical device
        void* m_LogicalDevice;
        void* m_GraphicsQueue;
        void* m_PresentationQueue;
        std::vector<const char*> m_DeviceExtensions;
        // swap chain
        void* m_SwapChain;
        std::vector<void*> m_SwapChainImages;
        int m_SwapChainImageFormat;
        Extent2D m_SwapChainExtent;
        std::vector<void*> m_ImageViews;
        // shaders
        std::vector<Shader> m_Shaders;
        // pipeline layout
        void* m_PipelineLayout;
        // render passes
        void* m_RenderPass;
        // pipeline
        void* m_Pipeline;
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