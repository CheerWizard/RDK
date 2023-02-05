#pragma once

#include <Core.h>

#include <vector>

namespace rdk {

    class DescriptorPool final {

    public:
        void create(VkDevice device, VkDescriptorPoolSize* poolSizes, u32 poolSizeCount, u32 maxSets);
        void destroy();

        void createSets(u32 count, const VkDescriptorSetLayout& layout);

        inline VkDescriptorSet& operator [](u32 i) { return m_Sets[i]; }

    private:
        VkDevice m_Device;
        VkDescriptorPool m_Handle;
        std::vector<VkDescriptorSet> m_Sets;
    };

}