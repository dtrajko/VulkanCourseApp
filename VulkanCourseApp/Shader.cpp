#include "Shader.h"

#include <fstream>


Shader::Shader(std::shared_ptr<DeviceLVE> device)
	: m_Device{ device }
{
}

Shader::~Shader()
{
    vkDestroyShaderModule(m_Device->device(), vertShaderModule, nullptr);
    vkDestroyShaderModule(m_Device->device(), fragShaderModule, nullptr);
}

std::vector<char> Shader::readFile(const std::string& filepath)
{
    std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void Shader::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(m_Device->device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
}
