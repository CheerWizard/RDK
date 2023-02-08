#include <CommandPool.h>

#include <stdexcept>

#define IO ImGui::GetIO()

namespace rdk {

    void CommandPool::create() {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = m_Queue->getFamilyIndices().graphicsFamily;
        auto status = vkCreateCommandPool(m_Device->getLogicalHandle(), &info, nullptr, &m_Handle);
        rect_assert(status == VK_SUCCESS, "Failed to create Vulkan command pool")
        createBuffers();
        createSyncObjects();
    }

    void CommandPool::destroy() {
        destroySyncObjects();
        destroyBuffers();
        vkDestroyCommandPool(m_Device->getLogicalHandle(), m_Handle, nullptr);
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

        VkDevice device = m_Device->getLogicalHandle();
        for (int i = 0 ; i < m_MaxFramesInFlight ; i++) {
            auto imageAvailableStatus = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore[i]);
            auto renderFinishedStatus = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore[i]);
            auto flightFenceStatus = vkCreateFence(device, &fenceInfo, nullptr, &m_FlightFence[i]);

            rect_assert(imageAvailableStatus == VK_SUCCESS, "Failed to create Vulkan image available semaphore")
            rect_assert(renderFinishedStatus == VK_SUCCESS, "Failed to create Vulkan render finished semaphore")
            rect_assert(flightFenceStatus == VK_SUCCESS, "Failed to create Vulkan in flight fence")
        }
    }

    void CommandPool::destroySyncObjects() {
        VkDevice device = m_Device->getLogicalHandle();
        for (int i = 0 ; i < m_MaxFramesInFlight ; i++) {
            vkDestroySemaphore(device, m_ImageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(device,  m_RenderFinishedSemaphore[i], nullptr);
            vkDestroyFence(device,  m_FlightFence[i], nullptr);
        }
    }

    void CommandPool::createBuffers() {
        VkDevice logicalDevice = m_Device->getLogicalHandle();

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

    void CommandPool::beginFrame() {
#ifdef IMGUI
        ImGui::Render();
#endif
        SwapChain& swapChain = m_Pipeline->getSwapChain();
        VkSwapchainKHR swapChainHandle = swapChain.getHandle();
        VkDevice logicalDevice = m_Device->getLogicalHandle();
        VkFence& currentFence = m_FlightFence[m_CurrentFrame];
        VkSemaphore& currentImageAvailableSemaphore = m_ImageAvailableSemaphore[m_CurrentFrame];
        VkSemaphore& currentRenderFinishedSemaphore = m_RenderFinishedSemaphore[m_CurrentFrame];
        VkSurfaceKHR& surface = m_Surface;
        void* window = m_Window;
        QueueFamilyIndices& familyIndices = m_Queue->getFamilyIndices();

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
            recreateSwapChain();
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
        QueueFamilyIndices& familyIndices = m_Queue->getFamilyIndices();

#ifdef IMGUI
        renderUIDrawData();
#endif

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
        auto graphicsSubmitStatus = vkQueueSubmit(m_Queue->getGraphicsHandle(), 1, &submitInfo, currentFence);
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
        auto presentResult = vkQueuePresentKHR(m_Queue->getPresentationHandle(), &presentInfo);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_FrameBufferResized) {
            m_FrameBufferResized = true;
            recreateSwapChain();
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
            VkImageLayout oldLayout, VkImageLayout newLayout,
            u32 mipLevels
    ) {
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
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = mipLevels;

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
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
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

    void CommandPool::generateMipmaps(VkImage image, int width, int height, u32 mipLevels) {
        beginTempCommand();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        VkCommandBuffer commandBuffer = m_TempCommand;
        int mipW = width;
        int mipH = height;

        for (u32 i = 1 ; i < mipLevels ; i++) {
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            barrier.subresourceRange.baseMipLevel = i - 1;

            vkCmdPipelineBarrier(
                    m_TempCommand,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1,
                    &barrier
            );

            VkImageBlit blitRegion {};

            blitRegion.srcOffsets[0] = { 0, 0, 0 };
            blitRegion.srcOffsets[1] = { mipW, mipH, 1 };
            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcSubresource.mipLevel = i - 1;

            blitRegion.dstOffsets[0] = { 0, 0, 0 };
            blitRegion.dstOffsets[1] = { mipW > 1 ? mipW / 2 : 1, mipH > 1 ? mipH / 2 : 1, 1 };
            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstSubresource.mipLevel = i;

            vkCmdBlitImage(
                    commandBuffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blitRegion,
                    VK_FILTER_LINEAR
            );

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                    m_TempCommand,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1,
                    &barrier
            );

            if (mipW > 1)
                mipW /= 2;
            if (mipH > 1)
                mipH /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
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

        vkAllocateCommandBuffers(m_Device->getLogicalHandle(), &allocInfo, &m_TempCommand);

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

        VkQueue graphicsQueue = m_Queue->getGraphicsHandle();

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(m_Device->getLogicalHandle(), m_Handle, 1, &m_TempCommand);
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

    void CommandPool::renderUIDrawData(ImDrawData* drawData) {
        ImGui_ImplVulkan_RenderDrawData(drawData, getCurrentBuffer(), m_Pipeline->getHandle());
//        if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
//            GLFWwindow* backup_current_context = glfwGetCurrentContext();
//            ImGui::UpdatePlatformWindows();
//            ImGui::RenderPlatformWindowsDefault();
//            glfwMakeContextCurrent(backup_current_context);
//        }
    }

    void CommandPool::beginUI() {
        IO.DisplaySize = { (float) m_Window->getWidth(), (float) m_Window->getHeight() };
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void CommandPool::recreateSwapChain() {
        GLFWwindow* window = (GLFWwindow*) m_Window->getHandle();
        // handling minimization
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        m_Device->waitIdle();

#ifdef IMGUI

        ImGui_ImplVulkanH_CreateOrResizeWindow(
                m_Instance,
                m_Device->getPhysicalHandle(),
                m_Device->getLogicalHandle(),
                &m_ImGuiData,
                m_Queue->getFamilyIndices().graphicsFamily,
                nullptr,
                width,
                height,
                1
        );
        m_ImGuiData.FrameIndex = 0;

#endif

        m_Pipeline->getSwapChain().recreate(window, m_Surface, m_Queue->getFamilyIndices());
    }

}