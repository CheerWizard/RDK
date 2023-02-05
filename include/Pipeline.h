#pragma once

#include <Shader.h>
#include <SwapChain.h>

#include <memory>

namespace rdk {

    enum LayoutBinding {
        VERTEX_UNIFORM_BUFFER,
        FRAG_UNIFORM_BUFFER,
        VERTEX_SAMPLER,
        FRAG_SAMPLER
    };

    class Pipeline final {

    public:
        inline void setLogicalDevice(VkDevice logicalDevice) {
            m_LogicalDevice = logicalDevice;
        }

        inline void setSwapChain(SwapChain* swapChain) {
            m_SwapChain = swapChain;
        }

        inline SwapChain& getSwapChain() {
            return *m_SwapChain;
        }

        void setAssemblyInput(VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        void setVertexInput(const VertexInput& vertexInput);
        void setDynamicStates(const std::vector<VkDynamicState>& dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        });
        void setViewport(const VkExtent2D& extent);
        void setScissor(const VkExtent2D& extent);
        void setShader(const Shader& shader);
        void setRasterizer();
        void setMultisampling();
        void setColorBlendAttachment();
        void setColorBlending();
        void setLayout();
        void createLayout();
        void setVertexBuffer(Buffer* vertexBuffer);
        void setIndexBuffer(Buffer* indexBuffer);

        VkDescriptorSetLayout createDescriptorLayout(VkDescriptorSetLayoutBinding* bindings, size_t count);

        VkDescriptorSetLayoutBinding createBinding(u32 binding, LayoutBinding bindingType);

        void create();
        void destroy();
        void destroyDescriptorLayout();

        void beginRenderPass(VkCommandBuffer commandBuffer, u32 imageIndex);
        void endRenderPass(VkCommandBuffer commandBuffer);

        void bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSet);

        void setViewPort(VkCommandBuffer commandBuffer);
        void setScissor(VkCommandBuffer commandBuffer);

        void drawVertices(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount);
        void drawIndices(VkCommandBuffer commandBuffer, u32 indexCount, u32 instanceCount);

    private:
        VkPipeline m_Handle;
        VkDevice m_LogicalDevice;

        SwapChain* m_SwapChain = nullptr;

        Buffer* m_VertexBuffer;
        Buffer* m_IndexBuffer;

        VkGraphicsPipelineCreateInfo m_Info{};
        VkPipelineDynamicStateCreateInfo m_DynamicState{};
        std::vector<VkDynamicState> m_DynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineInputAssemblyStateCreateInfo m_InputAssembly{};
        VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
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

        VkDescriptorSetLayout m_DescriptorSetLayout;
        VkDescriptorSetLayoutCreateInfo m_DescriptorSetLayoutInfo{};
    };

}