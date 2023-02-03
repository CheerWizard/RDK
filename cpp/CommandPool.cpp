#include <CommandPool.h>

namespace rdk {

    void CommandPool::create() {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = m_Queue.getFamilyIndices().graphicsFamily;
        auto status = vkCreateCommandPool(m_Device.getLogicalHandle(), &info, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command pool")
        createBuffers();
        createSyncObjects();
    }

    void CommandPool::destroy() {
        destroySyncObjects();
        destroyBuffers();
        vkDestroyCommandPool(m_Device.getLogicalHandle(), m_Handle, nullptr);
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
            auto imageAvailableStatus = vkCreateSemaphore(m_Device.getLogicalHandle(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore[i]);
            auto renderFinishedStatus = vkCreateSemaphore(m_Device.getLogicalHandle(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore[i]);
            auto flightFenceStatus = vkCreateFence( m_Device.getLogicalHandle(), &fenceInfo, nullptr, &m_FlightFence[i]);

            rect_assert(imageAvailableStatus == VK_SUCCESS, "Failed to create Vulkan image available semaphore")
            rect_assert(renderFinishedStatus == VK_SUCCESS, "Failed to create Vulkan render finished semaphore")
            rect_assert(flightFenceStatus == VK_SUCCESS, "Failed to create Vulkan in flight fence")
        }
    }

    void CommandPool::destroySyncObjects() {
        for (int i = 0 ; i < m_MaxFramesInFlight ; i++) {
            vkDestroySemaphore(m_Device.getLogicalHandle(), m_ImageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(m_Device.getLogicalHandle(),  m_RenderFinishedSemaphore[i], nullptr);
            vkDestroyFence(m_Device.getLogicalHandle(),  m_FlightFence[i], nullptr);
        }
    }

    void CommandPool::drawFrame(const DrawData& drawData, u32 instanceCount) {
        SwapChain& swapChain = m_Pipeline.getSwapChain();
        VkSwapchainKHR swapChainHandle = swapChain.getHandle();
        VkPhysicalDevice physicalDevice = m_Device.getPhysicalHandle();
        VkDevice logicalDevice = m_Device.getLogicalHandle();
        VkFence& currentFence = m_FlightFence[m_CurrentFrame];
        VkSemaphore& currentImageAvailableSemaphore = m_ImageAvailableSemaphore[m_CurrentFrame];
        VkSemaphore& currentRenderFinishedSemaphore = m_RenderFinishedSemaphore[m_CurrentFrame];
        VkSurfaceKHR& surface = m_Surface;
        void* window = m_Window;
        QueueFamilyIndices& familyIndices = m_Queue.getFamilyIndices();

        vkWaitForFences(logicalDevice, 1, &currentFence, VK_TRUE, UINT64_MAX);
        // fetch swap chain image
        uint32_t imageIndex;
        auto fetchResult = vkAcquireNextImageKHR(
                logicalDevice,
                swapChainHandle,
                UINT64_MAX,
                currentImageAvailableSemaphore,
                VK_NULL_HANDLE,
                &imageIndex
        );
        // validate fetch result
        if (fetchResult == VK_ERROR_OUT_OF_DATE_KHR) {
            swapChain.recreate(window, physicalDevice, surface, familyIndices);
            return;
        }
        rect_assert(fetchResult == VK_SUCCESS || fetchResult == VK_SUBOPTIMAL_KHR, "Failed to acquire Vulkan swap chain image")
        // Only reset the fence if we are submitting work
        vkResetFences(logicalDevice, 1, &currentFence);
        // record command into buffer
        // begin command buffer
        auto& commandBuffer = m_Buffers[m_CurrentFrame];
        commandBuffer.reset();
        commandBuffer.begin();
        VkCommandBuffer commandBufferHandle = commandBuffer.getHandle();
        // begin render pass
        m_Pipeline.beginRenderPass(commandBufferHandle, imageIndex);
        // prepare pipeline
        m_Pipeline.bind(commandBufferHandle);
        m_Pipeline.setViewPort(commandBufferHandle);
        m_Pipeline.setScissor(commandBufferHandle);
        // bind VBO
        m_Pipeline.bindVBO(commandBufferHandle);
        // draw
        m_Pipeline.draw(commandBufferHandle, drawData.vertexCount, instanceCount);
        // end render pass
        m_Pipeline.endRenderPass(commandBufferHandle);
        // end command buffer
        commandBuffer.end();
        // setup submit info
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkCommandBuffer commandBuffers[] = { commandBufferHandle };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = commandBuffers;

        VkSemaphore waitSemaphores[] = { currentImageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        VkSemaphore signalSemaphores[] = { currentRenderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        // submit graphics queue
        auto graphicsSubmitStatus = vkQueueSubmit(m_Queue.getGraphicsHandle(), 1, &submitInfo, currentFence);
        rect_assert(graphicsSubmitStatus == VK_SUCCESS, "Failed to submit Vulkan graphics queue")
        // presentation info
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChainHandle };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        // submit presentation queue
        auto presentResult = vkQueuePresentKHR(m_Queue.getPresentationHandle(), &presentInfo);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_FrameBufferResized) {
            m_FrameBufferResized = true;
            swapChain.recreate(window, physicalDevice, surface, familyIndices);
        } else if (presentResult != VK_SUCCESS) {
            rect_assert(false, "Failed to present Vulkan swap chain image")
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFramesInFlight;
    }

    void CommandPool::createBuffers() {
        VkDevice logicalDevice = m_Device.getLogicalHandle();

        m_Buffers.resize(m_MaxFramesInFlight);
        std::vector<VkCommandBuffer> buffers(m_MaxFramesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_Handle;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<u32>(buffers.size());
        auto status = vkAllocateCommandBuffers(logicalDevice, &allocInfo, buffers.data());
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command buffers")

        for (int i = 0 ; i < buffers.size() ; i++) {
            auto& buffer = m_Buffers[i];
            buffer.setHandle(buffers[i]);
            buffer.setLogicalDevice(logicalDevice);
        }
    }

    void CommandPool::destroyBuffers() {
        VkCommandPool commandPool = m_Handle;
        for (auto& buffer : m_Buffers) {
            buffer.destroy(commandPool);
        }
        m_Buffers.clear();
    }

    CommandPool::CommandPool(void* window, VkSurfaceKHR surface, const Device& device)
    : m_Window(window), m_Surface(surface), m_Device(device) {
        m_Queue.create(m_Device.getLogicalHandle(), m_Device.findQueueFamily(m_Surface));
    }

    void CommandBuffer::create(VkCommandPool commandPool, u32 count) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = count;
        auto status = vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command buffers")
    }

    void CommandBuffer::destroy(VkCommandPool commandPool, u32 count) {
        vkFreeCommandBuffers(m_LogicalDevice, commandPool, count, &m_Handle);
    }

    void CommandBuffer::begin() {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional
        auto status = vkBeginCommandBuffer(m_Handle, &beginInfo);
        rect_assert(status == VK_SUCCESS, "Failed to begin Vulkan command buffer")
    }

    void CommandBuffer::end() {
        vkEndCommandBuffer(m_Handle);
    }

    void CommandBuffer::reset() {
        vkResetCommandBuffer(m_Handle, 0);
    }

}