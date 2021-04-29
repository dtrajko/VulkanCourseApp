#include "Shader.h"

#include <fstream>


Shader::Shader(std::shared_ptr<DeviceLVE> device, const std::string& filepathVertex, const std::string& filepathFragment)
	: m_Device{ device }
{
    printf("---- Creating shader [ '%s', '%s' ]\n", filepathVertex.c_str(), filepathFragment.c_str());

    // Read in SPIR-V code of shaders
    auto vertexShaderCode = readFile(filepathVertex);
    auto fragmentShaderCode = readFile(filepathFragment);

    // printf("Vulkan SPIR-V shader code successfully loaded.\n");

    // Build Shader Modules to link to Graphics Pipeline
    // Create Shader Modules
    m_ShaderModuleVertex = createShaderModule(vertexShaderCode);
    m_ShaderModuleFragment = createShaderModule(fragmentShaderCode);
}

Shader::~Shader()
{
    vkDestroyShaderModule(m_Device->device(), m_ShaderModuleVertex, nullptr);
    vkDestroyShaderModule(m_Device->device(), m_ShaderModuleFragment, nullptr);
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

VkShaderModule Shader::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();                                 // Size of code
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // Pointer to code (of uint32_t pointer type)

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(m_Device->device(), &shaderModuleCreateInfo, nullptr, &shaderModule);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Shader Module!");
    }

    printf("Vulkan Shader Module successfully created.\n");

    return shaderModule;
}
