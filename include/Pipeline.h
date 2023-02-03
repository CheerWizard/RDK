#pragma once

#include <Shader.h>
#include <SwapChain.h>

#include <memory>

namespace rdk {

    class Pipeline final {

    public:
        void prepare();

        inline void setLogicalDevice(VkDevice logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }
        inline void setSwapChain(const SwapChain& swapChain) {
            m_SwapChain = swapChain;
        }

        inline SwapChain& getSwapChain() {
            return m_SwapChain;
        }

        inline void setVBO(const VertexBuffer& vbo) {
            m_Vbo = vbo;
        }

        void setShader(const Shader& shader);
        void create();

        void destroy();

        void beginRenderPass(VkCommandBuffer commandBuffer, u32 imageIndex);
        void endRenderPass(VkCommandBuffer commandBuffer);

        void bind(VkCommandBuffer commandBuffer);
        void bindVBO(VkCommandBuffer commandBuffer);

        void setViewPort(VkCommandBuffer commandBuffer);
        void setScissor(VkCommandBuffer commandBuffer);

        void draw(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount);

    private:
        VkPipeline m_Handle;
        VkDevice m_LogicalDevice;
        SwapChain m_SwapChain;
        VertexBuffer m_Vbo;

        VkGraphicsPipelineCreateInfo m_Info{};
        VkPipelineDynamicStateCreateInfo m_DynamicState{};
        std::vector<VkDynamicState> m_DynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineInputAssemblyStateCreateInfo m_InputAssembly{};
        VkViewport m_Viewport{};
        VkRect2D m_Scissor{};
        VkPipelineViewportStateCreateInfo m_ViewportState{};
        VkPipelineRasterizationStateCreateInfo m_Rasterizer{};
        VkPipelineMultisampleStateCreateInfo m_Multisample{};
        VkPipelineDepthStencilStateCreateInfo m_DepthStencil{};
        VkPipelineColorBlendAttachmentState m_ColorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo m_ColorBlending{};
        std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;

        VkPipelineLayout m_Layout;
        VkPipelineLayoutCreateInfo m_LayoutInfo{};

    };

}