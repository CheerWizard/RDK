#include <CommandPool.h>
#include <stdexcept>

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

    CommandPool::CommandPool(void* window, VkSurfaceKHR surface, const Device& device, DescriptorPool* descriptorPool)
    : m_Window(window), m_Surface(surface), m_Device(device), m_DescriptorPool(descriptorPool) {
        m_Queue.create(m_Device.getLogicalHandle(), m_Device.findQueueFamily(m_Surface));
    }

    void CommandPool::beginFrame() {
        SwapChain& swapChain = m_Pipeline->getSwapChain();
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
        auto fetchResult = vkAcquireNextImageKHR(
                logicalDevice,
                swapChainHandle,
                UINT64_MAX,
                currentImageAvailableSemaphore,
                VK_NULL_HANDLE,
                &currentImageIndex
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

        auto& pipeline = *m_Pipeline;

        // begin render pass
        pipeline.beginRenderPass(commandBufferHandle, currentImageIndex);
        // prepare pipeline
        VkDescriptorSet& descriptorSet = m_DescriptorPool->operator[](m_CurrentFrame);
        pipeline.bind(commandBufferHandle, &descriptorSet);
        pipeline.setViewPort(commandBufferHandle);
        pipeline.setScissor(commandBufferHandle);
    }

    void CommandPool::endFrame() {
        auto& pipeline = *m_Pipeline;
        auto& commandBuffer = m_Buffers[m_CurrentFrame];
        VkCommandBuffer commandBufferHandle = commandBuffer.getHandle();
        VkFence& currentFence = m_FlightFence[m_CurrentFrame];
        VkSemaphore& currentImageAvailableSemaphore = m_ImageAvailableSemaphore[m_CurrentFrame];
        VkSemaphore& currentRenderFinishedSemaphore = m_RenderFinishedSemaphore[m_CurrentFrame];
        SwapChain& swapChain = pipeline.getSwapChain();
        VkSwapchainKHR swapChainHandle = swapChain.getHandle();
        VkSurfaceKHR& surface = m_Surface;
        void* window = m_Window;
        QueueFamilyIndices& familyIndices = m_Queue.getFamilyIndices();
        VkPhysicalDevice physicalDevice = m_Device.getPhysicalHandle();

        // end render pass
        pipeline.endRenderPass(commandBufferHandle);
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

        presentInfo.pImageIndices = &currentImageIndex;
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

    void CommandPool::drawVertices(u32 vertexCount, u32 instanceCount) {
        m_Pipeline->drawVertices(m_Buffers[m_CurrentFrame].getHandle(), vertexCount, instanceCount);
    }

    void CommandPool::drawIndices(u32 indexCount, u32 instanceCount) {
        m_Pipeline->drawIndices(m_Buffers[m_CurrentFrame].getHandle(), indexCount, instanceCount);
    }

    void CommandPool::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        beginTempCommand();

        // copy buffers
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(m_TempCommand, srcBuffer, dstBuffer, 1, &copyRegion);

        endTempCommand();
    }

    void CommandPool::transitionImageLayout(
            VkImage image, VkFormat format,
            VkImageLayout oldLayout, VkImageLayout newLayout) {

        beginTempCommand();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("CommandPool::transitionImageLayout: old/new layouts are not supported!");
        }

        vkCmdPipelineBarrier(
                m_TempCommand,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );

        endTempCommand();
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

    VkCommandBuffer& CommandPool::beginTempCommand() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_Handle;
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(m_Device.getLogicalHandle(), &allocInfo, &m_TempCommand);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_TempCommand, &beginInfo);

        return m_TempCommand;
    }

    void CommandPool::endTempCommand() {
        vkEndCommandBuffer(m_TempCommand);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_TempCommand;

        VkQueue graphicsQueue = m_Queue.getGraphicsHandle();

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(m_Device.getLogicalHandle(), m_Handle, 1, &m_TempCommand);
    }

    void CommandPool::copyBufferImage(VkBuffer srcBuffer, VkImage dstImage, u32 width, u32 height) {
        beginTempCommand();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.baseArrayLayer = 0;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(
                m_TempCommand,
                srcBuffer,
                dstImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
        );

        endTempCommand();
    }

}