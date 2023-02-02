#include <Pipeline.h>

namespace rdk {

    void Pipeline::create() {
        // setup dynamic state for pipeline
        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        // setup input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        // setup viewport
        auto extent = m_SwapChain.getExtent();
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) extent.width;
        viewport.height = (float) extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        // setup scissor
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        VkExtent2D extent2D{};
        extent2D.width = extent.width;
        extent2D.height = extent.height;
        scissor.extent = extent2D;
        // setup viewport state
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        // setup rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
        // setup multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional
        // setup depth/stencil state
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        // setup color blend attachments
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        // setup color blend state
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional
        // setup pipeline layout info
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        // create pipeline layout
        auto pipelineLayoutStatus = vkCreatePipelineLayout((VkDevice) m_LogicalDevice, &pipelineLayoutInfo, nullptr, (VkPipelineLayout*) &m_Layout);
        rect_assert(pipelineLayoutStatus == VK_SUCCESS, "Failed to create Vulkan pipeline layout")
        // setup pipeline info
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        // setup shader stages
        Shader& exampleShader = m_Shaders.at(0);
        // vertex shader stage
        VkPipelineShaderStageCreateInfo vkVertStage {};
        ShaderStage vertStage = exampleShader.getVertStage();
        vkVertStage.sType = (VkStructureType) vertStage.sType;
        vkVertStage.pNext = vertStage.next;
        vkVertStage.module = (VkShaderModule) vertStage.module;
        vkVertStage.flags = vertStage.flags;
        vkVertStage.pName = vertStage.name;
        vkVertStage.pSpecializationInfo = (VkSpecializationInfo*) vertStage.specInfo;
        vkVertStage.stage = (VkShaderStageFlagBits) vertStage.stage;
        // fragment shader stage
        VkPipelineShaderStageCreateInfo vkFragStage {};
        ShaderStage fragStage = exampleShader.getFragStage();
        vkFragStage.sType = (VkStructureType) fragStage.sType;
        vkFragStage.pNext = fragStage.next;
        vkFragStage.module = (VkShaderModule) fragStage.module;
        vkFragStage.flags = fragStage.flags;
        vkFragStage.pName = fragStage.name;
        vkFragStage.pSpecializationInfo = (VkSpecializationInfo*) fragStage.specInfo;
        vkFragStage.stage = (VkShaderStageFlagBits) fragStage.stage;
        // setup all components into pipeline info
        VkPipelineShaderStageCreateInfo shaderStages[2] = {
                vkVertStage,
                vkFragStage
        };
        pipelineInfo.pStages = shaderStages;
        // setup vertex input info into pipeline
        VertexBindDescriptor bindDescriptor = exampleShader.getVertexFormat().getDescriptor();
        VkVertexInputBindingDescription vkBindDescriptor {};
        vkBindDescriptor.binding = bindDescriptor.binding;
        vkBindDescriptor.stride = bindDescriptor.stride;
        vkBindDescriptor.inputRate = (VkVertexInputRate) bindDescriptor.inputRate;
        std::vector<VertexAttr> attrs = exampleShader.getVertexFormat().getAttrs();
        std::vector<VkVertexInputAttributeDescription> vkAttrs;
        for (const auto& attr : attrs) {
            VkVertexInputAttributeDescription vkAttr;
            vkAttr.binding = attr.binding;
            vkAttr.format = (VkFormat) attr.format;
            vkAttr.location = attr.location;
            vkAttr.offset = attr.offset;
            vkAttrs.emplace_back(vkAttr);
        }
        VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttrs.size());
        vertexInputInfo.pVertexBindingDescriptions = &vkBindDescriptor;
        vertexInputInfo.pVertexAttributeDescriptions = vkAttrs.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;

        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = (VkPipelineLayout) m_Layout;
        pipelineInfo.renderPass = (VkRenderPass) m_SwapChain.getRenderPass().getHandle();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional
        // create pipeline
        auto pipelineStatus = vkCreateGraphicsPipelines((VkDevice) m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, (VkPipeline*) &m_Handle);
        rect_assert(pipelineStatus == VK_SUCCESS, "Failed to create Vulkan pipeline")
    }

    void Pipeline::destroy() {
        m_Shaders.clear();
        m_SwapChain.destroy();
        vkDestroyPipeline((VkDevice) m_LogicalDevice, (VkPipeline) m_Handle, nullptr);
    }

    void Pipeline::addShader(const char* vertFilepath, const char* fragFilepath) {
        m_Shaders.emplace_back(m_LogicalDevice, vertFilepath, fragFilepath);
    }

    void Pipeline::beginRenderPass(void* commandBuffer, u32 imageIndex) {
        // setup info
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = (VkRenderPass) m_SwapChain.getRenderPass().getHandle();
        renderPassInfo.framebuffer = (VkFramebuffer) m_SwapChain.getFrameBuffer(imageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        // extent
        VkExtent2D extent;
        extent.width = m_SwapChain.getExtent().width;
        extent.height = m_SwapChain.getExtent().height;
        renderPassInfo.renderArea.extent = extent;
        // setup clear color
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass((VkCommandBuffer) commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Pipeline::endRenderPass(void *commandBuffer) {
        vkCmdEndRenderPass((VkCommandBuffer) commandBuffer);
    }

    void Pipeline::bind(void* commandBuffer) {
        vkCmdBindPipeline((VkCommandBuffer) commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline) m_Handle);
    }

    void Pipeline::setViewPort(void *commandBuffer) {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_SwapChain.getExtent().width);
        viewport.height = static_cast<float>(m_SwapChain.getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport((VkCommandBuffer) commandBuffer, 0, 1, (VkViewport*) &viewport);
    }

    void Pipeline::setScissor(void *commandBuffer) {
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        VkExtent2D extent;
        extent.width = m_SwapChain.getExtent().width;
        extent.height = m_SwapChain.getExtent().height;
        scissor.extent = extent;
        vkCmdSetScissor((VkCommandBuffer) commandBuffer, 0, 1, (VkRect2D*) &scissor);
    }

    void Pipeline::draw(void* commandBuffer, u32 vertexCount, u32 instanceCount) {
        vkCmdDraw((VkCommandBuffer) commandBuffer, vertexCount, instanceCount, 0, 0);
    }

    void Pipeline::addShader(const Shader &shader) {
        m_Shaders.emplace_back(shader);
    }

}