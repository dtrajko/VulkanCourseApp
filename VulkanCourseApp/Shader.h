#pragma once

#include "DeviceLVE.h"

#include <string>
#include <vector>


class Shader {

public:
	Shader(std::shared_ptr<DeviceLVE> device);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

private:
	static std::vector<char> readFile(const std::string& filepath);
	void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

private:
	std::shared_ptr<DeviceLVE> m_Device;

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

};
