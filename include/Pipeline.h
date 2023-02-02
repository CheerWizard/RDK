#pragma once

#include <Shader.h>
#include <SwapChain.h>

namespace rdk {

    class Pipeline final {

    public:
        void create();

        inline void setLogicalDevice(void* logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }
        inline void setSwapChain(const SwapChain& swapChain) {
            m_SwapChain = swapChain;
        }

        inline SwapChain& getSwapChain() {
            return m_SwapChain;
        }

        void destroy();
        void addShader(const char* vertFilepath, const char* fragFilepath);
        void addShader(const Shader& shader);

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

}