#pragma once

#include "DeviceLVE.h"
#include "Shader.h"
#include "SwapChainLVE.h"

#include <string>
#include <vector>


struct PipelineConfigInfo {
	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};

class PipelineVCA {

public:
	PipelineVCA(
		std::shared_ptr<DeviceLVE> device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo,
		std::shared_ptr<SwapChainLVE> swapChain,
		VkRenderPass& renderPass);
	~PipelineVCA();

	PipelineVCA(const PipelineVCA&) = delete;
	PipelineVCA& operator=(const PipelineVCA&) = delete;

	void bind(VkCommandBuffer commandBuffer);

	static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

private:
	static std::vector<char> readFile(const std::string& filepath);

	void createGraphicsPipeline(
		const std::string& filepathVertex,
		const std::string& filepathFragment,
		const PipelineConfigInfo& configInfo);

	std::shared_ptr<DeviceLVE> m_Device;
	std::shared_ptr<Shader> m_Shader;
	std::shared_ptr<SwapChainLVE> m_SwapChain;
	VkRenderPass m_RenderPass; // TODO: create RenderPassVCA class

	VkPipeline m_GraphicsPipeline;
	VkPipelineLayout m_PipelineLayout;

	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorSetLayout m_SamplerSetLayout;

	VkPushConstantRange m_PushConstantRange;

};
