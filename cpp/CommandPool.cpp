#include <CommandPool.h>

namespace rdk {

    void CommandPool::create() {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = m_FamilyIndices.graphicsFamily;
        auto status = vkCreateCommandPool((VkDevice) m_Device.getLogicalHandle(), &info, nullptr, (VkCommandPool*) &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command pool")
        createBuffers();
        createSyncObjects();
    }

    void CommandPool::destroy() {
        destroySyncObjects();
        destroyBuffers();
        vkDestroyCommandPool((VkDevice) m_Device.getLogicalHandle(), (VkCommandPool) m_Handle, nullptr);
        m_Pipeline.destroy();
    }

    void CommandPool::addCommandBuffer(const CommandBuffer &commandBuffer) {
        m_Buffers.emplace_back(commandBuffer);
    }

    void CommandPool::createSyncObjects() {
        m_ImageAvailableSemaphore.resize(m_MaxFramesInFlight);
        m_RenderFinishedSemaphore.resize(m_MaxFramesInFlight);
        m_FlightFence.resize(m_MaxFramesInFlight);
        // setup semaphore info
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        // setup fence info
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0 ; i < m_MaxFramesInFlight ; i++) {
            auto imageAvailableStatus = vkCreateSemaphore((VkDevice) m_Device.getLogicalHandle(), &semaphoreInfo, nullptr, (VkSemaphore*) &m_ImageAvailableSemaphore[i]);
            auto renderFinishedStatus = vkCreateSemaphore((VkDevice) m_Device.getLogicalHandle(), &semaphoreInfo, nullptr, (VkSemaphore*) &m_RenderFinishedSemaphore[i]);
            auto flightFenceStatus = vkCreateFence((VkDevice) m_Device.getLogicalHandle(), &fenceInfo, nullptr, (VkFence*) &m_FlightFence[i]);

            rect_assert(imageAvailableStatus == VK_SUCCESS, "Failed to create Vulkan image available semaphore")
            rect_assert(renderFinishedStatus == VK_SUCCESS, "Failed to create Vulkan render finished semaphore")
            rect_assert(flightFenceStatus == VK_SUCCESS, "Failed to create Vulkan in flight fence")
        }
    }

    void CommandPool::destroySyncObjects() {
        for (int i = 0 ; i < m_MaxFramesInFlight ; i++) {
            vkDestroySemaphore((VkDevice) m_Device.getLogicalHandle(), (VkSemaphore) m_ImageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore((VkDevice) m_Device.getLogicalHandle(), (VkSemaphore) m_RenderFinishedSemaphore[i], nullptr);
            vkDestroyFence((VkDevice) m_Device.getLogicalHandle(), (VkFence) m_FlightFence[i], nullptr);
        }
    }

    void CommandPool::drawFrame(u32 vertexCount, u32 instanceCount) {
        VkSwapchainKHR swapChain = (VkSwapchainKHR) m_Pipeline.getSwapChain().getHandle();
        vkWaitForFences((VkDevice) m_Device.getLogicalHandle(), 1, (VkFence*) &m_FlightFence[m_CurrentFrame], VK_TRUE, UINT64_MAX);
        // fetch swap chain image
        uint32_t imageIndex;
        auto fetchResult = vkAcquireNextImageKHR(
                (VkDevice) m_Device.getLogicalHandle(),
                swapChain, UINT64_MAX,
                (VkSemaphore) m_ImageAvailableSemaphore[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
        // validate fetch result
        if (fetchResult == VK_ERROR_OUT_OF_DATE_KHR) {
            m_Pipeline.getSwapChain().recreate(m_Window, m_Device.getPhysicalHandle(), m_Surface, m_FamilyIndices);
            return;
        }
        rect_assert(fetchResult == VK_SUCCESS || fetchResult == VK_SUBOPTIMAL_KHR, "Failed to acquire Vulkan swap chain image")
        // Only reset the fence if we are submitting work
        vkResetFences((VkDevice) m_Device.getLogicalHandle(), 1, (VkFence*) &m_FlightFence[m_CurrentFrame]);
        // record command into buffer
        // begin command buffer
        auto& commandBuffer = m_Buffers[m_CurrentFrame];
        commandBuffer.reset();
        commandBuffer.begin();
        void* commandBufferHandle = commandBuffer.getHandle();
        // begin render pass
        m_Pipeline.beginRenderPass(commandBufferHandle, imageIndex);
        // prepare pipeline
        m_Pipeline.bind(commandBufferHandle);
        m_Pipeline.setViewPort(commandBufferHandle);
        m_Pipeline.setScissor(commandBufferHandle);
        // draw
        m_Pipeline.draw(commandBufferHandle, vertexCount, instanceCount);
        // end render pass
        m_Pipeline.endRenderPass(commandBufferHandle);
        // end command buffer
        commandBuffer.end();
        // setup submit info
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkCommandBuffer commandBuffers[] = { (VkCommandBuffer) commandBufferHandle };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = commandBuffers;

        VkSemaphore waitSemaphores[] = { (VkSemaphore) m_ImageAvailableSemaphore[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        VkSemaphore signalSemaphores[] = { (VkSemaphore) m_RenderFinishedSemaphore[m_CurrentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        // submit graphics queue
        auto graphicsSubmitStatus = vkQueueSubmit((VkQueue) m_Queue.getGraphicsHandle(), 1, &submitInfo, (VkFence) m_FlightFence[m_CurrentFrame]);
        rect_assert(graphicsSubmitStatus == VK_SUCCESS, "Failed to submit Vulkan graphics queue")
        // presentation info
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        // submit presentation queue
        auto presentResult = vkQueuePresentKHR((VkQueue) m_Queue.getPresentationHandle(), &presentInfo);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_FrameBufferResized) {
            m_FrameBufferResized = true;
            m_Pipeline.getSwapChain().recreate(m_Window, m_Device.getPhysicalHandle(), m_Surface, m_FamilyIndices);
        } else if (presentResult != VK_SUCCESS) {
            rect_assert(false, "Failed to present Vulkan swap chain image")
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFramesInFlight;
    }

    void CommandPool::createBuffers() {
        m_Buffers.resize(m_MaxFramesInFlight);
        std::vector<VkCommandBuffer> buffers(m_MaxFramesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = (VkCommandPool) m_Handle;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(buffers.size());
        auto status = vkAllocateCommandBuffers((VkDevice) m_Device.getLogicalHandle(), &allocInfo, (VkCommandBuffer*) buffers.data());
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command buffers")

        for (int i = 0 ; i < buffers.size() ; i++) {
            m_Buffers[i].setHandle(buffers[i]);
            m_Buffers[i].setLogicalDevice(m_Device.getLogicalHandle());
        }
    }

    void CommandPool::destroyBuffers() {
        for (auto& buffer : m_Buffers) {
            buffer.destroy(m_Handle);
        }
        m_Buffers.clear();
    }

    CommandPool::CommandPool(void* window, void* surface, const Device& device)
    : m_Window(window), m_Surface(surface), m_Device(device) {
        QueueFamilyIndices queueFamilyIndices = m_Device.findQueueFamily(m_Surface);
        m_Queue.create(m_Device.getLogicalHandle(), queueFamilyIndices);
    }

    void CommandBuffer::create(void* commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = (VkCommandPool) commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        auto status = vkAllocateCommandBuffers((VkDevice) m_LogicalDevice, &allocInfo, (VkCommandBuffer*) &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command buffers")
    }

    void CommandBuffer::destroy(void* commandPool) {
        vkFreeCommandBuffers((VkDevice) m_LogicalDevice, (VkCommandPool) commandPool, 1, (VkCommandBuffer*) &m_Handle);
    }

    void CommandBuffer::begin() {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional
        auto status = vkBeginCommandBuffer((VkCommandBuffer) m_Handle, &beginInfo);
        rect_assert(status == VK_SUCCESS, "Failed to begin Vulkan command buffer")
    }

    void CommandBuffer::end() {
        vkEndCommandBuffer((VkCommandBuffer) m_Handle);
    }

    void CommandBuffer::reset() {
        vkResetCommandBuffer((VkCommandBuffer) m_Handle, 0);
    }

    void CommandBuffer::setHandle(void *handle) {
        m_Handle = handle;
    }

}