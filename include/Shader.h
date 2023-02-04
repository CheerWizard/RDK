#pragma once

#include <Buffer.h>

#include <string>

namespace rdk {

    class Shader final {

    public:
        Shader() = default;
        Shader(VkDevice logicalDevice, const std::string& vertFilepath, const std::string& fragFilepath);
        ~Shader();

    public:
        inline const VkPipelineShaderStageCreateInfo& getVertStage() const {
            return m_VertStage;
        }

        inline const VkPipelineShaderStageCreateInfo& getFragStage() const {
            return m_FragStage;
        }

        inline std::vector<VkPipelineShaderStageCreateInfo> getStages() const {
            return { m_VertStage, m_FragStage };
        }

    private:
        void cleanup();
        std::vector<char> readFile(const char* filepath);
        void createModule(const std::vector<char>& src, VkShaderModule* module);

    private:
        VkDevice m_LogicalDevice;

        VkPipelineShaderStageCreateInfo m_VertStage{};
        VkShaderModule m_VertModule;

        VkPipelineShaderStageCreateInfo m_FragStage{};
        VkShaderModule m_FragModule;
    };

}