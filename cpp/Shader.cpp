#include <Shader.h>

#include <fstream>

namespace rdk {

    Shader::Shader(void* logicalDevice, const std::string &vertFilepath, const std::string &fragFilepath) {
        m_LogicalDevice = logicalDevice;
        // read shader files into sources
        auto vertSrc = readFile("spirv/shader_vert.spv");
        auto fragSrc = readFile("spirv/shader_frag.spv");
        // create shader modules from sources
        m_VertStage = ShaderStage(m_LogicalDevice, vertSrc);
        m_FragStage = ShaderStage(m_LogicalDevice, fragSrc);
        // setup shader into pipeline
        // vertex shader
        m_VertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_VertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        m_VertStage.name = "main";
        // fragment shader
        m_FragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_FragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        m_FragStage.name = "main";
    }

    Shader::~Shader() {
        cleanup();
    }

    void Shader::cleanup() {
        m_VertStage.cleanup(m_LogicalDevice);
        m_FragStage.cleanup(m_LogicalDevice);
    }

    void ShaderStage::cleanup(void* logicalDevice) {
        if (module)
            vkDestroyShaderModule((VkDevice) logicalDevice, (VkShaderModule) module, nullptr);
        module = nullptr;
    }

    ShaderStage::ShaderStage(void* logicalDevice, const std::vector<char>& code) {
        // setup shader module info
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const u32*>(code.data());
        // create shader module
        auto shaderModuleStatus = vkCreateShaderModule((VkDevice) logicalDevice, &createInfo, nullptr, (VkShaderModule*) &module);
        rect_assert(shaderModuleStatus == VK_SUCCESS, "Failed to create Vulkan shader module \nCode: %s\n", code.data())
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
}