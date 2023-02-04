#include <Pipeline.h>

namespace rdk {

    void Pipeline::setShader(const Shader &shader) {
        m_ShaderStages = shader.getStages();
        m_Info.stageCount = m_ShaderStages.size();
        m_Info.pStages = m_ShaderStages.data();
    }

    void Pipeline::setVertexInput(const VertexInput &vertexInput) {
        m_VertexInputState = vertexInput.info;
    }

    void Pipeline::create() {
        m_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        m_Info.pVertexInputState = &m_VertexInputState;
        m_Info.pInputAssemblyState = &m_InputAssembly;
        m_Info.pViewportState = &m_ViewportState;
        m_Info.pRasterizationState = &m_Rasterizer;
        m_Info.pMultisampleState = &m_Multisample;
        m_Info.pDepthStencilState = &m_DepthStencil; // Optional
        m_Info.pColorBlendState = &m_ColorBlending;
        m_Info.pDynamicState = &m_DynamicState;
        m_Info.layout = m_Layout;
        m_Info.renderPass = m_SwapChain.getRenderPass().getHandle();
        m_Info.subpass = 0;
        m_Info.basePipelineHandle = VK_NULL_HANDLE; // Optional
        m_Info.basePipelineIndex = -1; // Optional

        auto pipelineStatus = vkCreateGraphicsPipelines(
                m_LogicalDevice,
                VK_NULL_HANDLE,
                1, &m_Info,
                nullptr, &m_Handle
        );
        rect_assert(pipelineStatus == VK_SUCCESS, "Failed to create Vulkan pipeline")
    }

    void Pipeline::destroy() {
        m_SwapChain.destroy();
        destroyDescriptorLayout();
        vkDestroyPipeline(m_LogicalDevice, m_Handle, nullptr);
    }

    void Pipeline::beginRenderPass(VkCommandBuffer commandBuffer, u32 imageIndex) {
        SwapChain& swapChain = m_SwapChain;
        // setup info
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapChain.getRenderPass().getHandle();
        renderPassInfo.framebuffer = swapChain.getFrameBuffer(imageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        // extent
        renderPassInfo.renderArea.extent = swapChain.getExtent();
        // setup clear color
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Pipeline::endRenderPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
    }

    void Pipeline::bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSet) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handle);
        m_VertexBuffer->bindVertex(commandBuffer);
        m_IndexBuffer->bindIndex(commandBuffer);
        vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_Layout,
                0, 1,
                descriptorSet,
                0, nullptr
        );
    }

    void Pipeline::setViewPort(VkCommandBuffer commandBuffer) {
        VkExtent2D extent = m_SwapChain.getExtent();
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    }

    void Pipeline::setScissor(VkCommandBuffer commandBuffer) {
        VkExtent2D extent = m_SwapChain.getExtent();
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void Pipeline::drawVertices(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount) {
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
    }

    void Pipeline::drawIndices(VkCommandBuffer commandBuffer, u32 indexCount, u32 instanceCount) {
        vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
    }

    void Pipeline::setDynamicStates(const std::vector<VkDynamicState>& dynamicStates) {
        m_DynamicStates = dynamicStates;
        m_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_DynamicState.dynamicStateCount = static_cast<u32>(m_DynamicStates.size());
        m_DynamicState.pDynamicStates = m_DynamicStates.data();
    }

    void Pipeline::setAssemblyInput(VkPrimitiveTopology topology) {
        m_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        m_InputAssembly.topology = topology;
        m_InputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    void Pipeline::setViewport(const VkExtent2D &extent) {
        m_Viewport.x = 0.0f;
        m_Viewport.y = 0.0f;
        m_Viewport.width = static_cast<float>(extent.width);
        m_Viewport.height = static_cast<float>(extent.height);
        m_Viewport.minDepth = 0.0f;
        m_Viewport.maxDepth = 1.0f;
        m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        m_ViewportState.viewportCount = 1;
        m_ViewportState.pViewports = &m_Viewport;
    }

    void Pipeline::setScissor(const VkExtent2D& extent) {
        m_Scissor.offset = {0, 0 };
        m_Scissor.extent = extent;
        m_ViewportState.scissorCount = 1;
        m_ViewportState.pScissors = &m_Scissor;
    }

    void Pipeline::setRasterizer() {
        m_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        m_Rasterizer.depthClampEnable = VK_FALSE;
        m_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        m_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        m_Rasterizer.lineWidth = 1.0f;
        m_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        m_Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        m_Rasterizer.depthBiasEnable = VK_FALSE;
        m_Rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        m_Rasterizer.depthBiasClamp = 0.0f; // Optional
        m_Rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    }

    void Pipeline::setMultisampling() {
        m_Multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        m_Multisample.sampleShadingEnable = VK_FALSE;
        m_Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_Multisample.minSampleShading = 1.0f; // Optional
        m_Multisample.pSampleMask = nullptr; // Optional
        m_Multisample.alphaToCoverageEnable = VK_FALSE; // Optional
        m_Multisample.alphaToOneEnable = VK_FALSE; // Optional
    }

    void Pipeline::setColorBlendAttachment() {
        m_ColorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        m_ColorBlendAttachment.blendEnable = VK_TRUE;
        m_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    void Pipeline::setColorBlending() {
        m_ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        m_ColorBlending.logicOpEnable = VK_FALSE;
        m_ColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        m_ColorBlending.attachmentCount = 1;
        m_ColorBlending.pAttachments = &m_ColorBlendAttachment;
        m_ColorBlending.blendConstants[0] = 0.0f; // Optional
        m_ColorBlending.blendConstants[1] = 0.0f; // Optional
        m_ColorBlending.blendConstants[2] = 0.0f; // Optional
        m_ColorBlending.blendConstants[3] = 0.0f; // Optional
    }

    void Pipeline::setLayout() {
        m_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        m_LayoutInfo.setLayoutCount = 1; // Optional
        m_LayoutInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional
        m_LayoutInfo.pushConstantRangeCount = 0; // Optional
        m_LayoutInfo.pPushConstantRanges = nullptr; // Optional
    }

    void Pipeline::createLayout() {
        auto pipelineLayoutStatus = vkCreatePipelineLayout(m_LogicalDevice, &m_LayoutInfo, nullptr, &m_Layout);
        rect_assert(pipelineLayoutStatus == VK_SUCCESS, "Failed to create Vulkan pipeline layout")
    }

    void Pipeline::setVertexBuffer(Buffer* vertexBuffer) {
        m_VertexBuffer = vertexBuffer;
    }

    void Pipeline::setIndexBuffer(Buffer* indexBuffer) {
        m_IndexBuffer = indexBuffer;
    }

    VkDescriptorSetLayout Pipeline::createDescriptorLayout() {
        m_DescriptorSetLayoutBinding.binding = 0;
        m_DescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        m_DescriptorSetLayoutBinding.descriptorCount = 1;
        m_DescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        m_DescriptorSetLayoutBinding.pImmutableSamplers = nullptr; // Optional

        m_DescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        m_DescriptorSetLayoutInfo.bindingCount = 1;
        m_DescriptorSetLayoutInfo.pBindings = &m_DescriptorSetLayoutBinding;

        auto status = vkCreateDescriptorSetLayout(m_LogicalDevice, &m_DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan descriptor set")

        return m_DescriptorSetLayout;
    }

    void Pipeline::destroyDescriptorLayout() {
        vkDestroyDescriptorSetLayout(m_LogicalDevice, m_DescriptorSetLayout, nullptr);
    }

}