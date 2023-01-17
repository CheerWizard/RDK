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

    class WindowListener {
    public:
        virtual void onFrameBufferResized(int width, int height) = 0;
    };

    class Window final {

    public:
        Window(const char* title, int width, int height, WindowListener* listener);
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

    class RenderPass final {

    public:
        void create();
        void destroy();
        inline void* getHandle();
        inline void setLogicalDevice(void* logicalDevice);
        inline void setFormat(int format);

    private:
        void* m_Handle;
        void* m_LogicalDevice;
        int m_Format;
    };

    class FrameBuffer final {

    public:
        void create(void* imageView, void* renderPass, const Extent2D& extent);
        void destroy();
        inline void* getHandle();
        inline void setLogicalDevice(void* logicalDevice);

    private:
        void* m_Handle;
        void* m_LogicalDevice;
    };

    class SwapChain final {

    public:
        void create(void* window, void* physicalDevice, void* surface, const QueueFamilyIndices& indices);
        void destroy();
        void queryImages(u32 imageCount);
        inline void* getHandle();
        inline void setLogicalDevice(void* logicalDevice);
        inline const Extent2D& getExtent();
        inline int getImageFormat() const;
        inline void setRenderPass(const RenderPass& renderPass);
        inline RenderPass& getRenderPass();
        inline void* getFrameBuffer(u32 imageIndex);
        void createImageViews();
        void createFrameBuffers();
        void recreate(void* window, void* physicalDevice, void* surface, const QueueFamilyIndices& indices);

    private:
        void* m_Handle;
        void* m_LogicalDevice;
        std::vector<void*> m_Images;
        std::vector<void*> m_ImageViews;
        int m_ImageFormat;
        Extent2D m_Extent;
        RenderPass m_RenderPass;
        std::vector<FrameBuffer> m_FrameBuffers;
    };

    class Pipeline final {

    public:
        void create();
        inline void setLogicalDevice(void* logicalDevice);
        inline void setSwapChain(const SwapChain& swapChain);
        inline SwapChain& getSwapChain();
        void destroy();
        void addShader(const char* vertFilepath, const char* fragFilepath);

        void beginRenderPass(void* commandBuffer, u32 imageIndex);
        void endRenderPass(void* commandBuffer);

        void bind(void* commandBuffer);

        void setViewPort(void* commandBuffer);
        void setScissor(void* commandBuffer);

        void draw(void* commandBuffer, u32 vertexCount, u32 instanceCount);

    private:
        void* m_Handle;
        void* m_LogicalDevice;
        void* m_Layout;
        std::vector<Shader> m_Shaders;
        SwapChain m_SwapChain;
    };

    class CommandBuffer final {

    public:
        void create(void* commandPool);
        void destroy(void* commandPool);
        inline void* getHandle();
        void setHandle(void* handle);
        void setLogicalDevice(void* logicalDevice);
        void begin();
        void end();
        void reset();

    private:
        void* m_Handle;
        void* m_LogicalDevice;
    };

    class CommandPool final {

    public:
        CommandPool() = default;
        CommandPool(void* window, void* physicalDevice, void* logicalDevice, void* surface, const QueueFamilyIndices& familyIndices)
        : m_Window(window), m_PhysicalDevice(physicalDevice), m_LogicalDevice(logicalDevice),
        m_Surface(surface), m_FamilyIndices(familyIndices) {}

    public:
        void create();
        inline void setGraphicsQueue(void* graphicsQueue);
        inline void setPresentationQueue(void* graphicsQueue);
        inline void setPipeline(const Pipeline& pipeline);
        void destroy();
        void addCommandBuffer(const CommandBuffer& commandBuffer);
        void drawFrame(u32 vertexCount, u32 instanceCount);
        inline void setMaxFramesInFlight(u32 maxFramesInFlight);
        inline void setFrameBufferResized(bool resized);

    private:
        void createBuffers();
        void destroyBuffers();
        void createSyncObjects();
        void destroySyncObjects();

    private:
        void* m_Handle;
        void* m_LogicalDevice;
        void* m_PhysicalDevice;
        void* m_Window;
        void* m_Surface;
        QueueFamilyIndices m_FamilyIndices;
        std::vector<CommandBuffer> m_Buffers;
        Pipeline m_Pipeline;
        // sync objects
        u32 m_MaxFramesInFlight = 2;
        u32 m_CurrentFrame = 0;
        bool m_FrameBufferResized = false;
        std::vector<void*> m_ImageAvailableSemaphore;
        std::vector<void*> m_RenderFinishedSemaphore;
        std::vector<void*> m_FlightFence;
        // queues
        void* m_GraphicsQueue;
        void* m_PresentationQueue;
    };

    class GraphicsInstance final {

    public:
        explicit GraphicsInstance(const AppInfo& appInfo, Window* window);
        ~GraphicsInstance();

    public:
        void printExtensions();
        void drawFrame(u32 vertexCount, u32 instanceCount);
        void onFrameBufferResized(int width, int height);

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
        std::vector<const char*> m_DeviceExtensions;
        // commands and pipeline
        CommandPool m_CommandPool;
    };

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
        GraphicsInstance* m_GraphicsInstance;
    };

}