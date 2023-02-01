#pragma once

#include <VertexFormat.h>
#include <string>

namespace rdk {

    struct ShaderStage final {
        int sType;
        const void* next = nullptr;
        u32 flags;
        u32 stage;
        void* module;
        const char* name;
        const void* specInfo = nullptr;

        ShaderStage() = default;
        ShaderStage(void* logicalDevice, const std::vector<char>& code);

        void cleanup(void* logicalDevice);
    };

    class Shader final {

    public:
        Shader() = default;
        Shader(void* logicalDevice, const std::string& vertFilepath, const std::string& fragFilepath);
        ~Shader();

    public:
        inline const ShaderStage& getVertStage() const {
            return m_VertStage;
        }

        inline const ShaderStage& getFragStage() const {
            return m_FragStage;
        }

    private:
        void cleanup();
        std::vector<char> readFile(const char* filepath);

    private:
        void* m_LogicalDevice = nullptr;
        VertexFormat m_VertexFormat;
        ShaderStage m_VertStage;
        ShaderStage m_FragStage;
    };

}