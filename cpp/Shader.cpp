#include <Shader.h>

#include <fstream>

namespace rdk {

    Shader::Shader(VkDevice logicalDevice, const std::string &vertFilepath, const std::string &fragFilepath) {
        m_LogicalDevice = logicalDevice;
        // setup vertex shader
        auto vertSrc = readFile(vertFilepath.c_str());
        createModule(vertSrc, &m_VertModule);
        m_VertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_VertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        m_VertStage.pName = "main";
        m_VertStage.module = m_VertModule;
        // setup fragment shader
        auto fragSrc = readFile(fragFilepath.c_str());
        createModule(fragSrc, &m_FragModule);
        m_FragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_FragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        m_FragStage.pName = "main";
        m_FragStage.module = m_FragModule;
    }

    Shader::~Shader() {
        cleanup();
    }

    void Shader::cleanup() {
        if (m_VertModule)
            vkDestroyShaderModule(m_LogicalDevice, m_VertModule, nullptr);
        m_VertModule = nullptr;

        if (m_FragModule)
            vkDestroyShaderModule(m_LogicalDevice, m_FragModule, nullptr);
        m_FragModule = nullptr;
    }

    std::vector<char> Shader::readFile(const char* filepath) {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);
        rect_assert(file.is_open(), "Failed to open file %s\n", filepath)

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void Shader::createModule(const std::vector<char>& src, VkShaderModule* module) {
        // setup shader module info
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = src.size();
        createInfo.pCode = reinterpret_cast<const u32*>(src.data());
        // create shader module
        auto shaderModuleStatus = vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, module);
        rect_assert(shaderModuleStatus == VK_SUCCESS, "Failed to create Vulkan shader module \nCode: %s\n", src.data())
    }
}