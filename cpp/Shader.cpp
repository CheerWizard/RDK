#include <Shader.h>

#include <shaderc/shaderc.hpp>

#include <fstream>
#include <iostream>

namespace rdk {

    static std::vector<char> readFile(const char* filepath) {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);
        rect_assert(file.is_open(), "Failed to open file %s\n", filepath)

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static void createModule(VkDevice device, const std::vector<u32>& spirvCode, VkShaderModule* module) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spirvCode.size() * sizeof(u32);
        createInfo.pCode = spirvCode.data();

        auto shaderModuleStatus = vkCreateShaderModule(device, &createInfo, nullptr, module);
        rect_assert(shaderModuleStatus == VK_SUCCESS, "Failed to create Vulkan shader module")
    }

    static shaderc_shader_kind getShaderType(const VkShaderStageFlagBits shaderType) {
        switch (shaderType) {
            case VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT:
                return shaderc_glsl_vertex_shader;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                return shaderc_glsl_tess_control_shader;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                return shaderc_glsl_tess_evaluation_shader;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT:
                return shaderc_glsl_geometry_shader;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT:
                return shaderc_glsl_fragment_shader;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT:
                return shaderc_glsl_compute_shader;
            default:
                throw std::runtime_error("getShaderType: unknown shader type");
        }
    }

    static std::vector<u32> compile(
            const char* filepath,
            const char* entryPointName,
            const VkShaderStageFlagBits shaderType
    ) {
        auto shaderCode = readFile(filepath);

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        shaderc::SpvCompilationResult spvModule;

        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        spvModule = compiler.CompileGlslToSpv(
                shaderCode.data(),
                shaderCode.size(),
                getShaderType(shaderType),
                filepath,
                entryPointName,
                options
        );

        if (spvModule.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << spvModule.GetErrorMessage();
            throw std::runtime_error("Shader::compile: Failed to compile GLSL into SPIR-V. Check error message above");
        }

        return { spvModule.begin(), spvModule.end() };
    }

    Shader::Shader(VkDevice logicalDevice, const std::string &vertFilepath, const std::string &fragFilepath) {
        m_LogicalDevice = logicalDevice;
        // setup vertex shader
        auto vertBytecode = compile(vertFilepath.c_str(), "main", VK_SHADER_STAGE_VERTEX_BIT);
        createModule(m_LogicalDevice, vertBytecode, &m_VertModule);
        m_VertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_VertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        m_VertStage.pName = "main";
        m_VertStage.module = m_VertModule;
        // setup fragment shader
        auto fragBytecode = compile(fragFilepath.c_str(), "main", VK_SHADER_STAGE_FRAGMENT_BIT);
        createModule(m_LogicalDevice, fragBytecode, &m_FragModule);
        m_FragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_FragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        m_FragStage.pName = "main";
        m_FragStage.module = m_FragModule;
    }

    Shader::~Shader() {
        cleanup();
    }

    void Shader::cleanup() {
        vkDestroyShaderModule(m_LogicalDevice, m_VertModule, nullptr);
        vkDestroyShaderModule(m_LogicalDevice, m_FragModule, nullptr);
    }
}