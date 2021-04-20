#pragma once

#include "DeviceLVE.h"

#include <string>
#include <vector>


class Shader {

public:
	Shader(std::shared_ptr<DeviceLVE> device, const std::string& filepathVertex, const std::string& filepathFragment);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	VkShaderModule& getShaderModuleVertex() { return m_ShaderModuleVertex; };
	VkShaderModule& getShaderModuleFragment() { return m_ShaderModuleFragment; };

private:
	static std::vector<char> readFile(const std::string& filepath);
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	std::shared_ptr<DeviceLVE> m_Device;

	VkShaderModule m_ShaderModuleVertex;
	VkShaderModule m_ShaderModuleFragment;

};
