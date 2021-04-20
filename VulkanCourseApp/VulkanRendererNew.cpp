#include "VulkanRendererNew.h"

#include "VulkanValidation.h"
#include "DeviceLVE.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <stdexcept>
#include <set>
#include <algorithm>
#include <array>


VulkanRendererNew::VulkanRendererNew(std::shared_ptr<WindowLVE> window)
	: m_Window{ window }
{
}

int VulkanRendererNew::init()
{
	try
	{
		createInstance();
		createDebugCallback();
		createDevice();
		// createSurface();
		// getPhysicalDevice();
		// createLogicalDevice();
		createSwapChain();
		// createRenderPass();
		createDescriptorSetLayout();
		createPushConstantRange();
		createGraphicsPipeline();
		// createColorBufferImage();
		// createDepthBufferImage();
		// createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createTextureSampler();
		// allocateDynamicBufferTransferSpace();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createInputDescriptorSets();
		createSynchronization();

		float aspectRatio = (float)m_SwapChain->getSwapChainExtent().width / (float)m_SwapChain->getSwapChainExtent().height;

		uboViewProjection.projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.f);
		uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 25.0f, 25.0f), glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Vulkan inverts Y axis
		uboViewProjection.projection[1][1] *= -1;

		// Create our default "no texture" texture
		createTexture("plain.png");
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRendererNew::updateModel(int modelId, glm::mat4 newModel)
{
	if (modelId >= modelList.size()) return;

	modelList[modelId].setModel(newModel);
}

void VulkanRendererNew::drawFrameLVE()
{
	uint32_t imageIndex;
	auto result = m_SwapChain->acquireNextImage(&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChainLVE();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	recordCommandBufferLVE(imageIndex);

	result = m_SwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window->wasWindowResized()) {
		m_Window->resetWindowResizedFlag();
		recreateSwapChainLVE();
		return;
	}

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}
}

void VulkanRendererNew::draw()
{
	uint32_t imageIndex;
	auto result = m_SwapChain->acquireNextImage(&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChainLVE();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	recordCommands(imageIndex);
	updateUniformBuffers(imageIndex);

	result = m_SwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

	/****
	// CPU/GPU synchronization
	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(m_Device->device(), 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// Manually reset (clpse) fences
	vkResetFences(m_Device->device(), 1, &drawFences[currentFrame]);

	// -- 1. GET NEXT IMAGE --
	// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
	// Get next available image to draw to and set something to signal when we're finished with the image (a semaphore)
	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_Device->device(), m_SwapChain->getSwapChainKHR(), std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

	recordCommands(imageIndex);
	updateUniformBuffers(imageIndex);

	// -- 2. SUBMIT COMMAND BUFFER TO RENDER
	// Queue submission information
	// Submit command buffer to queue for execution, making sure it waits for image to be signalled as available before drawing
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;                          // Number of semaphores to wait on
	submitInfo.pWaitSemaphores = &imageAvailable[currentFrame]; // List of semaphores to wait on
	VkPipelineStageFlags waitStages[] =
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	submitInfo.pWaitDstStageMask = waitStages;                    // Stages to check semaphores at
	submitInfo.commandBufferCount = 1;                            // Number of command buffers to submit
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];     // Command buffer to submit
	submitInfo.signalSemaphoreCount = 1;                          // Number of semaphores to signal
	submitInfo.pSignalSemaphores = &renderFinished[currentFrame]; // List of semaphores to signal when command buffer finishes

	// Submit command buffer to queue
	VkResult result = vkQueueSubmit(m_Device->graphicsQueue(), 1, &submitInfo, drawFences[currentFrame]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit a Command Buffer to Queue!");
	}

	// -- 3. PRESENT RENDERED IMAGE TO SCREEN
	// Present image to screen when it has signalled finished rendering
	// and signals when it has finished rendering
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;                          // Number of semaphores to wait on
	presentInfo.pWaitSemaphores = &renderFinished[currentFrame]; // List of semaphores to wait on
	presentInfo.swapchainCount = 1;                              // Number of swapchains to present to
	presentInfo.pSwapchains = &m_SwapChain->getSwapChainKHR();   // List of swapchains to present to
	presentInfo.pImageIndices = &imageIndex;                     // Index of images in swapchain to present

	// Present image
	result = vkQueuePresentKHR(m_Device->presentationQueue(), &presentInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present Image!");
	}

	// Get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	****/
}

void VulkanRendererNew::cleanup()
{
	// Wait until no actions being run on device before destroying
	vkDeviceWaitIdle(m_Device->device());

	// _aligned_free(modelTransferSpace);

	for (size_t i = 0; i < modelList.size(); i++)
	{
		modelList[i].destroyMeshModel();
	}
	modelList.clear();

	vkDestroySampler(m_Device->device(), textureSampler, nullptr);

	for (size_t i = 0; i < textureImages.size(); i++)
	{
		vkDestroyImage(m_Device->device(), textureImages[i], nullptr);
		vkFreeMemory(m_Device->device(), textureImageMemory[i], nullptr);
	}

	for (size_t i = 0; i < textureImageViews.size(); i++)
	{
		vkDestroyImageView(m_Device->device(), textureImageViews[i], nullptr);
	}

	//	for (size_t i = 0; i < depthBufferImages.size(); i++)
	//	{
	//		vkDestroyImageView(m_Device->device(), depthBufferImageViews[i], nullptr);
	//		vkDestroyImage(m_Device->device(), depthBufferImages[i], nullptr);
	//		vkFreeMemory(m_Device->device(), depthBufferImageMemory[i], nullptr);
	//	}

	//	for (size_t i = 0; i < colorBufferImages.size(); i++)
	//	{
	//		vkDestroyImageView(m_Device->device(), colorBufferImageViews[i], nullptr);
	//		vkDestroyImage(m_Device->device(), colorBufferImages[i], nullptr);
	//		vkFreeMemory(m_Device->device(), colorBufferImageMemory[i], nullptr);
	//	}

	vkDestroyDescriptorPool(m_Device->device(), descriptorPool, nullptr);
	vkDestroyDescriptorPool(m_Device->device(), samplerDescriptorPool, nullptr);
	vkDestroyDescriptorPool(m_Device->device(), inputDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_Device->device(), descriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_Device->device(), samplerSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_Device->device(), inputSetLayout, nullptr);

	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		vkDestroyBuffer(m_Device->device(), vpUniformBuffer[i], nullptr);
		vkFreeMemory(m_Device->device(), vpUniformBufferMemory[i], nullptr);

		// vkDestroyBuffer(m_Device->device(), modelDynUniformBuffer[i], nullptr);
		// vkFreeMemory(m_Device->device(), modelDynUniformBufferMemory[i], nullptr);
	}

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		vkDestroySemaphore(m_Device->device(), imageAvailable[i], nullptr);
		vkDestroySemaphore(m_Device->device(), renderFinished[i], nullptr);
		vkDestroyFence(m_Device->device(), drawFences[i], nullptr);
	}

	vkDestroyCommandPool(m_Device->device(), graphicsCommandPool, nullptr);
	//	for (auto framebuffer : m_SwapChain->getFrameBuffers())
	//	{
	//		vkDestroyFramebuffer(m_Device->device(), framebuffer, nullptr);
	//	}

	vkDestroyPipeline(m_Device->device(), graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device->device(), pipelineLayout, nullptr);

	vkDestroyPipeline(m_Device->device(), secondPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device->device(), secondPipelineLayout, nullptr);

	vkDestroyRenderPass(m_Device->device(), m_SwapChain->getRenderPass(), nullptr);

	for (auto& imageView : m_SwapChain->getSwapChainImageViews())
	{
		vkDestroyImageView(m_Device->device(), imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_Device->device(), m_SwapChain->getSwapChainKHR(), nullptr);
	// vkDestroySurfaceKHR(instance, m_Device->surface(), nullptr);
	// vkDestroyDevice(m_Device->device(), nullptr);
	if (validationEnabled)
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	// vkDestroyInstance(instance, nullptr);
}

VulkanRendererNew::~VulkanRendererNew()
{
	cleanup();
}

void VulkanRendererNew::createInstance()
{
	if (validationEnabled && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Required Validation Layers not supported!");
	}

	// Information about the application itself
	// Most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";                // Custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // Custom version of the application
	appInfo.pEngineName = "No Engine";                      // Custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);       // Custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_0;                // The Vulkan API version

	// Creation information for a VkInstance (Vulkan Instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// createInfo.pNext = nullptr;
	// createInfo.flags = VkInstanceCreateFlags();
	createInfo.pApplicationInfo = &appInfo;

	// Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Set up extensions the Instance will use
	uint32_t glfwExtensionCount = 0;    // GLFW may require multiple extensions
	const char** glfwExtensions;        // Extensions passed as array of cstrings, so need pointer (the array) to pointer (the string)

	// Get GLFW extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Add GLFW extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	// If validation enabled, add extension to report validation debug info
	if (validationEnabled)
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	// Check Instance Extensions supported
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("vkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// Set up Validation Layers that Instance will use
	if (validationEnabled)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	// Create Instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance!");
	}

	printf("Vulkan Instance successfully created.\n");
}

void VulkanRendererNew::createDebugCallback()
{
	// Only create callback if validation enabled
	if (!validationEnabled) return;

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT; // Which validation reports should initiate callback
	callbackCreateInfo.pfnCallback = debugCallback; // Pointer to callback function itself

	// Create debug callback with custom create function
	VkResult result = CreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Debug Callback!");
	}

	printf("Vulkan Debug Callback successfully created.\n");
}

void VulkanRendererNew::createDevice()
{
	m_Device = std::make_shared<DeviceLVE>(m_Window);
}

/****
void VulkanRenderer::createLogicalDevice()
{
	// Get the Queue Family indices for the chosen Physical Device
	QueueFamilyIndices indices = getQueueFamilies(m_Device->getPhysicalDevice());

	// Vector for queue creation information, and set for queue family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues the logical device needs to create and info to do so (Only 1 for now, will add more later!)
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;                       // Number of queues to create
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;  // The index of the queue family to create a queue from
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority; // Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Information to create logical device (sometimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());  // Number of Queue Create Infos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();                            // List of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); // Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();                      // List of enabled logical device extensions

	// Physical Device Features the Logical Device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE; // Enabling Anisotropy

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Physical Device features the Logical Device will use

	// Create the logical device for the given physical device
	VkResult result = vkCreateDevice(m_Device->getPhysicalDevice(), &deviceCreateInfo, nullptr, &m_Device->device());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Logical Device!");
	}

	// Queues are created at the same time as the device
	// so we want handle to queues
	// From given logical device, of given Queue Family, of given Queue Index (0 since only one queue),
	// place reference in given VkQueue
	vkGetDeviceQueue(m_Device->device(), indices.graphicsFamily,     0, &graphicsQueue);
	vkGetDeviceQueue(m_Device->device(), indices.presentationFamily, 0, &presentationQueue);

	printf("Vulkan Logical Device successfully created.\n");
}
****/

/****
void VulkanRenderer::createSurface()
{
	// Create Surface (creates a surface create info struct, runs the create surface function, returns result)
	VkResult result = glfwCreateWindowSurface(instance, m_Window->getHandle(), nullptr, m_Device->surface());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a surface!");
	}

	printf("Vulkan/GLFW Window Surface successfully created.\n");
}
****/

void VulkanRendererNew::createSwapChain()
{
	m_SwapChain = std::make_unique<SwapChainVCA>(m_Device, m_Window->getExtent(), nullptr, m_Window);

	/****
	// Get SwapChain details so we can pick best settings
	SwapChainLVE::SwapChainDetails swapChainDetails = getSwapChainDetails();

	// Find optimal surface values for our swap chain
	// 1. CHOOSE BEST SURFACE FORMAT
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	// 2. CHOOSE BEST PRESENTATION MODE
	VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
	// 3. CHOOSE SWAP CHAIN RESOLUTION
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

	// How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// If image count higher than max, then clamp down to max
	// If 0, then limitless
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
		swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	// Creation information for swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = m_Device->surface();                                        // Swapchain surface
	swapChainCreateInfo.imageFormat = surfaceFormat.format;                                   // Swapchain format
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;                           // Swapchain color space
	swapChainCreateInfo.presentMode = presentMode;                                            // Swapchain presentation mode
	swapChainCreateInfo.imageExtent = extent;                                                 // Swapchain image extents
	swapChainCreateInfo.minImageCount = imageCount;                                           // Minimum images in swapchain
	swapChainCreateInfo.imageArrayLayers = 1;                                                 // Number of layers for each image in chain
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                     // What attachment images will be used as
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform; // Transform to perform to swap chain images
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle blending images with external graphics (e.g. other windows)
	swapChainCreateInfo.clipped = VK_TRUE; // Whether to clip parts of image not in view (e.g. behind another window, off screen)

	// Get Queue Family Indices
	QueueFamilyIndices indices = getQueueFamilies();

	// If Graphics and Presentation families are different, then swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[] =
		{
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily,
		};

		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 2;                     // Number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;      // Array of queues to share between
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create Swapchain
	VkResult result = vkCreateSwapchainKHR(m_Device->device(), &swapChainCreateInfo, nullptr, &swapchain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Swapchain!");
	}

	printf("Vulkan Swapchain successfully created.\n");

	// Store for later reference
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(m_Device->device(), swapchain, &swapChainImageCount, nullptr);

	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_Device->device(), swapchain, &swapChainImageCount, images.data());

	for (VkImage image : images)
	{
		// Store image handle
		SwapChainLVE::SwapchainImage swapChainImage = {};
		swapChainImage.image = image;
		swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		// Add to swapchain image list
		swapChainImages.push_back(swapChainImage);
	}
	****/
}

/****
void VulkanRendererNew::createRenderPass()
{
	// Array of our subpasses
	std::array<VkSubpassDescription, 2> subpasses = {};

	// ATTACHMENTS

	// -- SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

	// -- -- Color Attachment (Input Attachment)
	VkFormat colorBufferImageFormat = chooseSupportedFormat(
		{ VK_FORMAT_R8G8B8A8_UNORM },
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = colorBufferImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// -- -- Depth Attachment (Input Attachment)
	VkFormat depthBufferImageFormat = chooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthBufferImageFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// -- -- Color Attachment (Input Attachment) Reference
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 1; // ID of attachments array in createFramebuffers
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// -- -- Depth Attachment (Input Attachment) Reference
	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 2; // ID of attachments array in createFramebuffers
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Set up Subpass 1
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount = 1;
	subpasses[0].pColorAttachments = &colorAttachmentReference;
	subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

	// -- SUBPASS 2 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

	// -- -- Swapchain Color Attachment
	VkAttachmentDescription swapchainColorAttachment = {};
	swapchainColorAttachment.format = m_SwapChain->getSwapChainImageFormat();   // Format to use for attachment
	swapchainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                   // Number of samples to write for multisampling
	swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;              // Describes what to do with attachment before rendering
	swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Describes what to do with attachment after rendering
	swapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
	swapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	swapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;         // Image data layout before Render Pass starts
	swapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;     // Image data layout after Render Pass (to change to)

	// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference swapchainColorAttachmentReference = {};
	swapchainColorAttachmentReference.attachment = 0; // First one from the render pass attachments list
	swapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// References to attachments that subpass will take input from
	std::array<VkAttachmentReference, 2> inputReferences;
	inputReferences[0].attachment = 1;
	inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[1].attachment = 2;
	inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Set up Subpass 2
	subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[1].colorAttachmentCount = 1;
	subpasses[1].pColorAttachments = &swapchainColorAttachmentReference;
	subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
	subpasses[1].pInputAttachments = inputReferences.data();

	// SUBPASS DEPENDENCIES

	// Need to determine when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 3> subpassDependencies;

	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED (colorAttachment.initialLayout) to 
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (colorAttachmentReference.layout)
	// Transition must happen after:
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of Render Pass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;           // Stage access mask (memory access)
	// But must happen before:
	subpassDependencies[0].dstSubpass = 0; // First subpass (?)
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// Subpass 1 layout (color/depth) to Subpass 2 layout
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstSubpass = 1;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (colorAttachmentReference.layout) to 
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (colorAttachment.finalLayout)
	// Transition must happen after:
	subpassDependencies[2].srcSubpass = 0; // Subpass index
	subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// But must happen before:
	subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[2].dependencyFlags = 0;

	std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColorAttachment, colorAttachment, depthAttachment };

	// Create info for Render Pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();
	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassCreateInfo.pSubpasses = subpasses.data();
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(m_Device->device(), &renderPassCreateInfo, nullptr, &m_SwapChain->getRenderPass());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Render Pass!");
	}

	printf("Vulkan Render Pass successfully created.\n");
}
****/

void VulkanRendererNew::createDescriptorSetLayout()
{
	// UNIFORM VALUES DESCRIPTOR SET LAYOUT
	// UboViewProjection Binding Info
	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0;                                        // Binding point in shader (designated by binding number in shader)
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor (uniform, dynamic uniform, image sampler, etc)
	vpLayoutBinding.descriptorCount = 1;                                // Number of descriptors for binding
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;            // Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr;                       // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

	// Model Binding Info (Model struct)
	// VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	// modelLayoutBinding.binding = 1;
	// modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// modelLayoutBinding.descriptorCount = 1;
	// modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	// modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding }; // vpLayoutBinding, modelLayoutBinding

	// Create Descriptor Set Layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size()); // Number of binding infos
	layoutCreateInfo.pBindings = layoutBindings.data();                           // Array of binding infos

	// Create Descriptor Set Layout
	VkResult result = vkCreateDescriptorSetLayout(m_Device->device(), &layoutCreateInfo, nullptr, &descriptorSetLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}

	printf("Vulkan Descriptor Set Layout (Uniforms) successfully created.\n");

	// CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
	// Texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	// Create a Descriptor Set Layout with given bindings for texture
	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	// Create Descriptor Set Layout
	result = vkCreateDescriptorSetLayout(m_Device->device(), &textureLayoutCreateInfo, nullptr, &samplerSetLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}

	printf("Vulkan Descriptor Set Layout (Samplers) successfully created.\n");

	// CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
	// Color Input Binding
	VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
	colorInputLayoutBinding.binding = 0;
	colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputLayoutBinding.descriptorCount = 1;
	colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Depth Input Binding
	VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
	depthInputLayoutBinding.binding = 1;
	depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputLayoutBinding.descriptorCount = 1;
	depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Array of input attachment bindings
	std::vector<VkDescriptorSetLayoutBinding> inputBindings = { colorInputLayoutBinding, depthInputLayoutBinding };

	// Create a descriptor set layout for input attachments
	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
	inputLayoutCreateInfo.pBindings = inputBindings.data();

	// Create Descriptor Set Layout
	result = vkCreateDescriptorSetLayout(m_Device->device(), &inputLayoutCreateInfo, nullptr, &inputSetLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}

	printf("Vulkan Descriptor Set Layout (Input Attachments) successfully created.\n");
}

void VulkanRendererNew::createPushConstantRange()
{
	// Define push constant values (no 'create' needed!)
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage push constant will go to
	pushConstantRange.offset = 0;                              // Offset into given data to pass to push constant
	pushConstantRange.size = sizeof(Model);                    // Size of data being passed
}

void VulkanRendererNew::createGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/vert.spv");
	auto fragmentShaderCode = readFile("Shaders/frag.spv");

	printf("Vulkan SPIR-V shader code successfully loaded.\n");

	// Build Shader Modules to link to Graphics Pipeline
	// Create Shader Modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

	// -- SHADER STAGE CREATION INFORMATION --
	// Vertex Stage creation information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;  // Shader Stage name
	vertexShaderCreateInfo.module = vertexShaderModule;         // Shader module to be used by stage
	vertexShaderCreateInfo.pName = "main";                      // Entry point into shader

	// Fragment Stage creation information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Shader Stage name
	fragmentShaderCreateInfo.module = fragmentShaderModule;        // Shader module to be used by stage
	fragmentShaderCreateInfo.pName = "main";                       // Entry point into shader

	// Put shader stage creation info into an array
	// Graphics Pipeline creation info requires array of shader stage creates
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// How the data for a single vertex (including info such as position, color, texture coords, normals etc) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;             // Can bind multiple streams of data, this defines which one
	bindingDescription.stride = sizeof(Vertex); // Size of a single vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // How to move between data after each vertex (related to instancing)
	                                                            // VK_VERTEX_INPUT_RATE_VERTEX   : Move on to the next vertex
	                                                            // VK_VERTEX_INPUT_RATE_INSTANCE : Move to a vertex for the next instance
	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

	// Position attribute
	attributeDescriptions[0].binding = 0;                         // Which binding the data is at (should be same as above)
	attributeDescriptions[0].location = 0;                        // Location in shader where data will be read from
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Format the data will take (also helps define size of data)
	attributeDescriptions[0].offset = offsetof(Vertex, pos);      // Where this attribute is defined in the data for a single vertex

	// Color attribute
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, col);

	// Texture Coords attribute
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tex);

	// -- VERTEX INPUT --
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;   // List of Vertex Binding Descriptions (data spacing/stride info)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // List of Vertex Attribute Descriptions (data format and where to bind to/from)

	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive type to assemble vertices as
	inputAssembly.primitiveRestartEnable = VK_FALSE;              // Allow overriding of "strip" topology to start new primitives

	// -- VIEWPORT & SCISSOR (think of GoldenEye 007 Nintendo 64) --
	// Create a viewport info struct
	VkViewport viewport = {};
	viewport.x = 0.0f;                                                 // x start coordinate
	viewport.y = 0.0f;                                                 // y start coordinate
	viewport.width  = (float)m_SwapChain->getSwapChainExtent().width;  // width of viewport
	viewport.height = (float)m_SwapChain->getSwapChainExtent().height; // height of viewport
	viewport.minDepth = 0.0f;                                          // min framebuffer depth
	viewport.maxDepth = 1.0f;                                          // max framebuffer depth

	// Create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };                          // Offset to use region from
	scissor.extent = m_SwapChain->getSwapChainExtent(); // Extent to describe region to use, starting at offset

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- DYNAMIC STATES --
	// Dynamic states to enable
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic Viewport: Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
 	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);  // Dynamic Scissor:  Can resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Dynamic State creation info
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;                 // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;          // Whether to discard data and skip rasterizer. 
	                                                                  // Never creates fragments, only suitable for pipeline without framebuffer output
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;          // How to handle filling points between vertices (Wireframe mode)
	rasterizerCreateInfo.lineWidth = 1.0f;                            // How thick lines should be when drawn
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;            // Which face of a triangle to cull
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;                  // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;               // Enable multisample shading or not
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to use per fragment

	// -- BLENDING --
	// Blend Attachment State (how blending is handled)
	VkPipelineColorBlendAttachmentState colorState = {};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | // Colors to apply blending to
		                        VK_COLOR_COMPONENT_G_BIT |
		                        VK_COLOR_COMPONENT_B_BIT |
		                        VK_COLOR_COMPONENT_A_BIT;
	colorState.blendEnable = VK_TRUE;                      // Enable blending

	// Blending uses equation: (srcColorBlendFactor * newColor); colorBlendOp(dstColorBlendFactor * oldColor)
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.colorBlendOp = VK_BLEND_OP_ADD;

	// Summarized: (VK_BLEND_FACTOR_SRC_ALPHA * newColor) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColor)
	// (newColor.alpha * newColor) + ((1 - newColor.alpha) * oldColor)
	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;
	// Summarized: (1 * newAlpha) + (0 * oldAlpha) = newAlpha

	// Blending decides how to blend a new color being written to a fragment, with the old value
	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE; // Alternative to calculations is to use logical operations
	// colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorState;

	// -- PIPELINE LAYOUT --
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout };

	// Decriptor Sets and Push Constants
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	// Create Pipeline Layout
	VkResult result = vkCreatePipelineLayout(m_Device->device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout (1st Subpass)!");
	}

	printf("Vulkan Pipeline Layout (1st Subpass) successfully created.\n");

	// -- DEPTH STENCIL TESTING --
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;           // Enable checking depth to determine fragment write
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;          // Enable writing to depth buffer (to replace old values)
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS; // Comparison operation that allows an overwrite (is in front)
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;    // Depth Bounds Test: Does the depth value exists between two bounds
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;        // Enable Stencil Test

	// -- GRAPHICS PIPELINE CREATION
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2; // Number of shader stages (number of elements in shaderStages array)
	pipelineCreateInfo.pStages = shaderStages;                     // List of shader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo; // All the fixed function pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState   = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState    = &colorBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;                   // Pipeline Layout the pipeline should use
	pipelineCreateInfo.renderPass = m_SwapChain->getRenderPass(); // Render Pass description the pipeline is compatible with (Pipeline is used by the Render Pass)
	pipelineCreateInfo.subpass = 0;                               // Subpass of Render Pass to use with the pipeline

	// Pipeline Derivatives: Can create multiple pipelines that derive from one another for optimization
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Existing pipeline to derive from
	pipelineCreateInfo.basePipelineIndex = -1; // or index of pipeline being created to derive from (in case creating multiple at once)

	// Create Graphics Pipeline
	result = vkCreateGraphicsPipelines(m_Device->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline (1st Subpass)!");
	}

	printf("Vulkan Graphics Pipeline (1st Subpass) successfully created.\n");

	// Destroy Shader Modules, no longer needed after Pipeline created
	vkDestroyShaderModule(m_Device->device(), fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_Device->device(), vertexShaderModule,   nullptr);

	// CREATE SECOND SUBPASS PIPELINE
	// Second subpass shaders
	auto secondVertexShaderCode   = readFile("Shaders/second_vert.spv");
	auto secondFragmentShaderCode = readFile("Shaders/second_frag.spv");

	// Build shaders
	VkShaderModule secondVertexShaderModule   = createShaderModule(secondVertexShaderCode);
	VkShaderModule secondFragmentShaderModule = createShaderModule(secondFragmentShaderCode);

	// Set new shaders
	vertexShaderCreateInfo.module = secondVertexShaderModule;
	fragmentShaderCreateInfo.module = secondFragmentShaderModule;

	VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// No vertex data for second pass
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

	// Don't want to write to depth buffer
	depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

	// Create new pipeline layout
	VkPipelineLayoutCreateInfo secondPipelineLayoutCreateInfo = {};
	secondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	secondPipelineLayoutCreateInfo.setLayoutCount = 1;
	secondPipelineLayoutCreateInfo.pSetLayouts = &inputSetLayout;
	secondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	secondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// Create Pipeline Layout (2nd Subpass) 
	result = vkCreatePipelineLayout(m_Device->device(), &secondPipelineLayoutCreateInfo, nullptr, &secondPipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout (2nd Subpass)!");
	}

	printf("Vulkan Pipeline Layout (2nd Subpass) successfully created.\n");

	pipelineCreateInfo.pStages = secondShaderStages;  // Update second shader stage list
	pipelineCreateInfo.layout = secondPipelineLayout; // Change pipeline layout for input attachment descriptor sets
	pipelineCreateInfo.subpass = 1;                   // Use 2nd Subpass (starting at 0)

	// Create second pipeline
	result = vkCreateGraphicsPipelines(m_Device->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &secondPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline (2nd Subpass)!");
	}

	printf("Vulkan Graphics Pipeline (2nd Subpass) successfully created.\n");

	// Destroy second shader modules
	vkDestroyShaderModule(m_Device->device(), secondVertexShaderModule, nullptr);
	vkDestroyShaderModule(m_Device->device(), secondFragmentShaderModule, nullptr);
}

/****
void VulkanRendererNew::createColorBufferImage()
{
	// Resize supported format for color attachment
	colorBufferImages.resize(m_SwapChain->getSwapChainImages().size());
	colorBufferImageMemory.resize(m_SwapChain->getSwapChainImages().size());
	colorBufferImageViews.resize(m_SwapChain->getSwapChainImages().size());

	// Get supported format for color attachment
	VkFormat colorBufferImageFormat = chooseSupportedFormat(
		{ VK_FORMAT_B8G8R8A8_UNORM }, // VK_FORMAT_R8G8B8A8_UNORM
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		// Create Color Buffer Image
		colorBufferImages[i] = createImage(m_SwapChain->getSwapChainExtent().width, m_SwapChain->getSwapChainExtent().height, colorBufferImageFormat, 
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&colorBufferImageMemory[i]);

		// Create Color Buffer Image View
		colorBufferImageViews[i] = createImageView(colorBufferImages[i], colorBufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}
****/

/****
void VulkanRendererNew::createDepthBufferImage()
{
	depthBufferImages.resize(m_SwapChain->getSwapChainImages().size());
	depthBufferImageMemory.resize(m_SwapChain->getSwapChainImages().size());
	depthBufferImageViews.resize(m_SwapChain->getSwapChainImages().size());

	// Get supported format for depth buffer
	VkFormat depthBufferImageFormat = chooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		// Create Depth Buffer Image
		depthBufferImages[i] = createImage(m_SwapChain->getSwapChainExtent().width, m_SwapChain->getSwapChainExtent().height, depthBufferImageFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&depthBufferImageMemory[i]);

		// Create Depth Buffer Image View
		depthBufferImageViews[i] = createImageView(depthBufferImages[i], depthBufferImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
}
****/

/****
void VulkanRendererNew::createFramebuffers()
{
	// Resize framebuffer count to equal swapchain image count
	m_SwapChain->getFrameBuffers().resize(m_SwapChain->getSwapChainImages().size());

	// Create a framebuffer for each swapchain image
	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		std::array<VkImageView, 3> attachments =
		{
			m_SwapChain->getSwapChainImageViews()[i],
			colorBufferImageViews[i],
			depthBufferImageViews[i],
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_SwapChain->getRenderPass();           // Render Pass layout the Framebuffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();                   // List of attachments (1:1 with Render Pass)
		framebufferCreateInfo.width = m_SwapChain->getSwapChainExtent().width;     // Framebuffer width
		framebufferCreateInfo.height = m_SwapChain->getSwapChainExtent().height;   // Framebuffer height
		framebufferCreateInfo.layers = 1;                                          // Framebuffer layers

		VkResult result = vkCreateFramebuffer(m_Device->device(), &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Framebuffer!");
		}

		printf("Vulkan Framebuffer successfully created.\n");
	}
}
****/

void VulkanRendererNew::createCommandPool()
{
	// Get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = m_Device->findPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily; // Queue Family type that buffers from this command pool will use

	// Create a Graphics Queue Family Command Pool
	VkResult result = vkCreateCommandPool(m_Device->device(), &poolInfo, nullptr, &graphicsCommandPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Command Pool!");
	}

	printf("Vulkan Graphics Command Pool successfully created.\n");
}

void VulkanRendererNew::createCommandBuffers()
{
	// Resize command buffer count to have one for each framebuffer
	commandBuffers.resize(m_SwapChain->getFrameBuffers().size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = graphicsCommandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // VK_COMMAND_BUFFER_LEVEL_PRIMARY   : Buffer you submit directly to queue. 
	                                                     // Can't be called by other buffers.
	                                                     // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't be called directly. 
	                                                     // Can be called from other buffers via "vkCmdExecuteCommands(buffer)"
	                                                     // when recording commands in primary buffer
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	// Allocate Command Buffers and place handles in array of buffers
	// Command Buffers are automatically deallocated when Command Pool is destroyed (on vkDestroyCommandPool)
	VkResult result = vkAllocateCommandBuffers(m_Device->device(), &cbAllocInfo, commandBuffers.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}

	printf("Vulkan Command Buffers successfully allocated from the Command Pool.\n");
}

void VulkanRendererNew::createUniformBuffers()
{
	// UboViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

	// Model struct buffer size
	// VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

	// One uniform buffer for each image (and by extension, command buffer)
	vpUniformBuffer.resize(m_SwapChain->getSwapChainImages().size());
	vpUniformBufferMemory.resize(m_SwapChain->getSwapChainImages().size());

	// modelDynUniformBuffer.resize(swapChainImages.size());
	// modelDynUniformBufferMemory.resize(swapChainImages.size());

	// Create Uniform buffer
	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VkMemoryPropertyFlags bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		createBuffer(m_Device->getPhysicalDevice(), m_Device->device(), vpBufferSize, bufferUsage, bufferProperties, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

		// createBuffer(mainDevice.physicalDevice, m_Device->device(), modelBufferSize, bufferUsage, bufferProperties, &modelDynUniformBuffer[i], &modelDynUniformBufferMemory[i]);
	}
}

void VulkanRendererNew::createDescriptorPool()
{
	// CREATE UNIFORM DESCRIPTOR POOL

	// Type of descriptors + how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)

	// UboViewProjection Pool
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

	// Model struct Pool (DYNAMIC)
	// VkDescriptorPoolSize modelPoolSize = {};
	// modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDynUniformBuffer.size());

	// List of Pool Sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize }; // vpPoolSize, modelPoolSize

	// Data to create Descriptor Pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapChain->getSwapChainImages().size()); // Maximum number of Descriptor Sets that can be created from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());         // Amount of Pool Sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();                                   // Pool SIzes to create pool with

	// Create Descriptor Pool
	VkResult result = vkCreateDescriptorPool(m_Device->device(), &poolCreateInfo, nullptr, &descriptorPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}

	printf("Vulkan Descriptor Pool (Uniforms) successfully created.\n");


	// CREATE SAMPLER DESCRIPTOR POOL
	// Texture sampler pool
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_TEXTURES;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_TEXTURES;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	result = vkCreateDescriptorPool(m_Device->device(), &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}

	printf("Vulkan Descriptor Pool (Samplers) successfully created.\n");


	// CREATE INPUT ATTACHMENT DESCRIPTOR POOL
	// Color Attachment Pool Size
	VkDescriptorPoolSize colorInputPoolSize = {};
	colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputPoolSize.descriptorCount = static_cast<uint32_t>(m_SwapChain->getColorBufferImageViews().size());

	VkDescriptorPoolSize depthInputPoolSize = {};
	depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputPoolSize.descriptorCount = static_cast<uint32_t>(m_SwapChain->getDepthBufferImageViews().size());

	std::vector<VkDescriptorPoolSize> inputPoolSizes = { colorInputPoolSize, depthInputPoolSize };

	// Create input attachment pool
	VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
	inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	inputPoolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapChain->getSwapChainImages().size());
	inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
	inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

	result = vkCreateDescriptorPool(m_Device->device(), &inputPoolCreateInfo, nullptr, &inputDescriptorPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}

	printf("Vulkan Descriptor Pool (Input Attachments) successfully created.\n");
}

void VulkanRendererNew::createDescriptorSets()
{
	// Resize Descriptor Set list so one for every buffer
	descriptorSets.resize(m_SwapChain->getSwapChainImages().size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_SwapChain->getSwapChainImages().size(), descriptorSetLayout);

	// Descriptor Set Allocation Info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descriptorPool;                                                      // Pool to allocate Descriptor Set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChain->getSwapChainImages().size()); // Number of sets to allocate
	setAllocInfo.pSetLayouts = setLayouts.data();                                                      // Layouts to use to allocate sets (1:1 relationship)

	// Allocate descriptor sets
	VkResult result = vkAllocateDescriptorSets(m_Device->device(), &setAllocInfo, descriptorSets.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Descriptor Sets!");
	}

	printf("Vulkan Descriptor Sets successfully allocated from the Descriptor Pool.\n");

	// Update all of descriptor set buffer bindings
	// Connect descriptorSets and uniformBuffer(s)
	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		// VIEW PROJECTION DESCRIPTOR UboViewProjection
		// Buffer info and data offset info
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = vpUniformBuffer[i];       // Buffer to get data from
		vpBufferInfo.offset = 0;                        // Position of start of data
		vpBufferInfo.range = sizeof(UboViewProjection); // Size of data

		// Data about connection between binding and buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = descriptorSets[i];                         // Descriptor Set to update
		vpSetWrite.dstBinding = 0;                                     // Binding to update (matches with binding on layout/shader)
		vpSetWrite.dstArrayElement = 0;                                // Index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor
		vpSetWrite.descriptorCount = 1;                                // Amount to update
		vpSetWrite.pBufferInfo = &vpBufferInfo;                        // Information about buffer data to bind

		// MODEL DESCRIPTOR
		// Model Buffer Binding Info (Model struct)
		// VkDescriptorBufferInfo modelBufferInfo = {};
		// modelBufferInfo.buffer = modelDynUniformBuffer[i]; // Buffer to get data from
		// modelBufferInfo.offset = 0;                        // Position of start of data
		// modelBufferInfo.range = modelUniformAlignment;     // Size of data

		// VkWriteDescriptorSet modelSetWrite = {};
		// modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// modelSetWrite.dstSet = descriptorSets[i];                                 // Descriptor Set to update
		// modelSetWrite.dstBinding = 1;                                             // Binding to update (matches with binding on layout/shader)
		// modelSetWrite.dstArrayElement = 0;                                        // Index in array to update
		// modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // Type of descriptor
		// modelSetWrite.descriptorCount = 1;                                        // Amount to update
		// modelSetWrite.pBufferInfo = &modelBufferInfo;                             // Information about buffer data to bind

		// List of Descriptor Set Writes
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite }; // vpSetWrite, modelSetWrite

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(m_Device->device(), static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

void VulkanRendererNew::createInputDescriptorSets()
{
	// Resize array to hold descriptor set for each swap chain image
	inputDescriptorSets.resize(m_SwapChain->getSwapChainImages().size());

	// Fill array of layouts ready for set creation
	std::vector<VkDescriptorSetLayout> setLayouts(m_SwapChain->getSwapChainImages().size(), inputSetLayout);

	// Input Attachment Descriptor Set Allocation Info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = inputDescriptorPool;
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChain->getSwapChainImages().size());
	setAllocInfo.pSetLayouts = setLayouts.data();

	// Allocate Descriptor Set
	VkResult result = vkAllocateDescriptorSets(m_Device->device(), &setAllocInfo, inputDescriptorSets.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Descriptor Sets (Input Attachments)!");
	}

	printf("Vulkan Descriptor Sets (Input Attachments) successfully allocated from the Descriptor Pool.\n");

	// Update each descriptor set with input attachment
	for (size_t i = 0; i < m_SwapChain->getSwapChainImages().size(); i++)
	{
		// Color Attachment Descriptor
		VkDescriptorImageInfo colorAttachmentDescriptor = {};
		colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorAttachmentDescriptor.imageView = m_SwapChain->getColorBufferImageViews()[i];
		colorAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Color Attachment Descriptor Write
		VkWriteDescriptorSet colorWrite = {};
		colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colorWrite.dstSet = inputDescriptorSets[i];
		colorWrite.dstBinding = 0;
		colorWrite.dstArrayElement = 0;
		colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colorWrite.descriptorCount = 1;
		colorWrite.pImageInfo = &colorAttachmentDescriptor;

		// Depth Attachment Descriptor
		VkDescriptorImageInfo depthAttachmentDescriptor = {};
		depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentDescriptor.imageView = m_SwapChain->getDepthBufferImageViews()[i];
		depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Depth Attachment Descriptor Write
		VkWriteDescriptorSet depthWrite = {};
		depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		depthWrite.dstSet = inputDescriptorSets[i];
		depthWrite.dstBinding = 1;
		depthWrite.dstArrayElement = 0;
		depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthWrite.descriptorCount = 1;
		depthWrite.pImageInfo = &depthAttachmentDescriptor;

		// List of input descriptor writes
		std::vector<VkWriteDescriptorSet> setWrites = { colorWrite, depthWrite };

		// Update descriptor sets
		vkUpdateDescriptorSets(m_Device->device(), static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

void VulkanRendererNew::recreateSwapChainLVE()
{
	auto extent = m_Window->getExtent();

	while (extent.width == 0 || extent.height == 0) {
		extent = m_Window->getExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_Device->device());

	if (m_SwapChain == nullptr) {
		m_SwapChain = std::make_unique<SwapChainVCA>(m_Device, extent, nullptr, m_Window);
	}
	else {
		m_SwapChain = std::make_unique<SwapChainVCA>(m_Device, extent, std::move(m_SwapChain), m_Window);
		freeCommandBuffers();
		createCommandBuffers();
	}

	// if render pass compatible do nothing else
	createGraphicsPipeline();
}

void VulkanRendererNew::freeCommandBuffers()
{
	vkFreeCommandBuffers(
		m_Device->device(),
		m_Device->getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());

	commandBuffers.clear();
}

void VulkanRendererNew::updateUniformBuffers(uint32_t imageIndex)
{
	// Copy ViewProjection data (UboViewProjection)
	void* data;
	vkMapMemory(m_Device->device(), vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(m_Device->device(), vpUniformBufferMemory[imageIndex]);

	// Copy Model data (Model struct)
	// for (size_t i = 0; i < meshList.size(); i++)
	// {
	// 	Model* thisModel = (Model*)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
	// 	*thisModel = meshList[i].getModel();
	// }

	// Map the list of model data
	// vkMapMemory(m_Device->device(), modelDynUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
	// memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
	// vkUnmapMemory(m_Device->device(), modelDynUniformBufferMemory[imageIndex]);
}

void VulkanRendererNew::recordCommands(uint32_t currentImage)
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_SwapChain->getRenderPass(); // Render Pass to begin
	renderPassBeginInfo.renderArea.offset = { 0, 0 };              // Start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = m_SwapChain->getSwapChainExtent(); // Size of region to run render pass on (starting at offset)

	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].color = { 0.29f, 0.14f, 0.35f, 1.0f };
	clearValues[2].depthStencil.depth = 1.0f;

	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size()); // Count of clear values
	renderPassBeginInfo.pClearValues = clearValues.data();                           // List of clear values

	renderPassBeginInfo.framebuffer = m_SwapChain->getFrameBuffer(currentImage);

	// Start recording commands to command buffer
	VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to start recording a Command Buffer!");
	}

	// printf("Command Buffer begin recording.\n");

	{
		// Begin Render Pass
		vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		{
			// Start 1st Subpass

			// Bind Pipeline to be used in Render Pass (1st Subpass)
			vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			for (size_t j = 0; j < modelList.size(); j++)
			{
				MeshModel thisModel = modelList[j];

				// "Push" constants to given shader stage directly (no buffer)
				VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				vkCmdPushConstants(
					commandBuffers[currentImage],
					pipelineLayout,
					stageFlags,           // Stage to push constants to
					0,                    // Offset of push constants to update
					sizeof(Model),        // Size of data being pushed
					&thisModel.getModel() // Actual data being pushed (can be array)
				);

				// printf("MeshModel->getMeshCount: %i\n", (int)thisModel.getMeshCount());

				for (size_t k = 0; k < thisModel.getMeshCount(); k++)
				{
					// printf("MeshModel->getIndexCount: %i\n", thisModel.getMesh(k)->getIndexCount());

					// Bind Vertex Buffer
					VkBuffer vertexBuffers[] = { thisModel.getMesh(k)->getVertexBuffer() }; // Vertex Buffers to bind
					VkDeviceSize offsets[] = { 0 };                                         // Offsets into buffers being bound
					vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers, offsets); // Command to bind vertex buffer before drawing with them

					// Bind mesh Index Buffer, with 0 offset and using the uint32 type
					VkBuffer indexBuffer = thisModel.getMesh(k)->getIndexBuffer(); // Index Buffer to bind
					VkDeviceSize offset = 0;                                       // Offsets into buffers being bound
					vkCmdBindIndexBuffer(commandBuffers[currentImage], indexBuffer, offset, VK_INDEX_TYPE_UINT32);

					// Dynamic Offset Amount
					uint32_t dynamicOffset = 0;
					uint32_t* dynamicOffsetPointer = nullptr;
					uint32_t dynamicOffsetCount = 0; // value 0 for Push Constants
					// dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * (uint32_t)j;
					// dynamicOffsetPointer = &dynamicOffset;
					// dynamicOffsetCount = 1; // value 1 for Uniform Buffer Dynamic

					std::array<VkDescriptorSet, 2> descriptorSetGroup =
					{
						descriptorSets[currentImage],
						samplerDescriptorSets[thisModel.getMesh(k)->getTexId()]
					};

					// Bind Descriptor Sets (Uniform Buffers and Texture Samplers)
					vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
						static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), dynamicOffsetCount, dynamicOffsetPointer);

					// Execute pipeline
					// vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(firstMesh.getVertexCount()), 1, 0, 0);
					vkCmdDrawIndexed(commandBuffers[currentImage], static_cast<uint32_t>(thisModel.getMesh(k)->getIndexCount()), 1, 0, 0, 0);
				}
			}

			// Bind vertices
			// vkCmdDraw - another draw command for different geometry

			// vkCmdBindPipeline - Bind another pipeline
		}

		{
			// Stard 2nd Subpass

			vkCmdNextSubpass(commandBuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipeline);

			// Bind Descriptor Sets (Input Attachment Descriptor Set)
			vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipelineLayout, 0,
				1, &inputDescriptorSets[currentImage], 0, nullptr);

			vkCmdDraw(commandBuffers[currentImage], 3, 1, 0, 0);
		}

		// End Render Pass
		vkCmdEndRenderPass(commandBuffers[currentImage]);
	}

	// Stop recording to command buffer
	result = vkEndCommandBuffer(commandBuffers[currentImage]);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to stop recording a Command Buffer!");
	}

	// printf("Command Buffer end recording.\n");
}

void VulkanRendererNew::recordCommandBufferLVE(int imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_SwapChain->getRenderPass();
	renderPassInfo.framebuffer = m_SwapChain->getFrameBuffer(imageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_SwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_SwapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(m_SwapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, m_SwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

	renderGameObjectsLVE(commandBuffers[imageIndex]);

	vkCmdEndRenderPass(commandBuffers[imageIndex]);

	if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer (imageIndex = " + std::to_string(imageIndex) + ")!");
	}
}

void VulkanRendererNew::renderGameObjectsLVE(VkCommandBuffer commandBuffer)
{
	m_Pipeline->bind(commandBuffer);

	/****
	for (auto& obj : m_GameObjectsLVE)
	{
		obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.01f, glm::two_pi<float>());

		SimplePushConstantData push{};
		push.offset = obj.transform2d.translation;
		push.color = obj.color;
		push.transform = obj.transform2d.mat2();

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);

		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer);
	}
	****/
}

/****
void VulkanRenderer::getPhysicalDevice()
{
	// Enumerate Physical Devices the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// If no devices available, then none support Vulkan!
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");
	}

	// Get list of Physical Devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for (const auto& device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}	

	// Get properties of our new device
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);

	// minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
}
****/

void VulkanRendererNew::allocateDynamicBufferTransferSpace()
{
	// 0000000100000000 256
	// 0000000001000000 64
	// 0000000011111111 256-1
	// 1111111100000000 ~(256-1) mask for all possible values
	// 0000000100000000 (64+256-1) & ~(256-1)

	// Calculate alignment of model data
	// modelUniformAlignment = (sizeof(Model) + minUniformBufferOffset - 1) & ~(minUniformBufferOffset - 1);

	// Hard limit for memory
	// Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
	// modelTransferSpace = (Model*)_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);

	// printf("allocateDynamicBufferTransferSpace: Model struct size = %i\n", (int)sizeof(Model));
	// printf("allocateDynamicBufferTransferSpace: minUniformBufferOffset = %i\n", (int)minUniformBufferOffset);
	// printf("allocateDynamicBufferTransferSpace: modelUniformAlignment = %i\n", (int)modelUniformAlignment);
}

bool VulkanRendererNew::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// Need to get number of extensions to create array of correct size to hold extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in list of available extensions
	for (const auto& checkExtension : *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRendererNew::checkDeviceExtensionSupport()
{
	// Get device extension cout
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(m_Device->getPhysicalDevice(), nullptr, &extensionCount, nullptr);

	// If no extensions found, return failure
	if (extensionCount == 0)
	{
		return false;
	}

	// Populate list of extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(m_Device->getPhysicalDevice(), nullptr, &extensionCount, extensions.data());

	// Check for extension
	for (const auto& deviceExtension : deviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRendererNew::checkValidationLayerSupport()
{
	// Get number of validation layers to create vector of appropriate size
	uint32_t validationLayerCount;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

	// Check if no validation layers found AND we want at least 1 layer
	if (validationLayerCount == 0 && validationLayers.size() > 0)
	{
		return false;
	}

	std::vector<VkLayerProperties> availableLayers(validationLayerCount);
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayers.data());

	// Check if given Validation Layer is in list of given Validation Layers
	for (const auto& validationLayer : validationLayers)
	{
		bool hasLayer = false;
		for (const auto& availableLayer : availableLayers)
		{
			if (strcmp(validationLayer, availableLayer.layerName) == 0)
			{
				hasLayer = true;
				break;
			}
		}

		if (!hasLayer)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRendererNew::checkDeviceSuitable()
{
	// Information about the device itself (ID, name, type, verdor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_Device->getPhysicalDevice(), &deviceProperties);

	printf("deviceProperties.deviceName: %s\n", deviceProperties.deviceName);
	printf("deviceProperties.limits.maxBoundDescriptorSets: %i\n", deviceProperties.limits.maxBoundDescriptorSets);
	printf("deviceProperties.limits.maxColorAttachments: %i\n", deviceProperties.limits.maxColorAttachments);
	printf("deviceProperties.limits.maxDescriptorSetSamplers: %i\n", deviceProperties.limits.maxDescriptorSetSamplers);
	printf("deviceProperties.limits.maxDescriptorSetUniformBuffers: %i\n", deviceProperties.limits.maxDescriptorSetUniformBuffers);
	printf("deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic: %i\n", deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic);
	printf("deviceProperties.limits.maxFramebufferWidth: %i\n", deviceProperties.limits.maxFramebufferWidth);
	printf("deviceProperties.limits.maxFramebufferHeight: %i\n", deviceProperties.limits.maxFramebufferHeight);
	printf("deviceProperties.limits.maxFramebufferLayers: %i\n", deviceProperties.limits.maxFramebufferLayers);
	printf("deviceProperties.limits.minUniformBufferOffsetAlignment: %i\n", (int)deviceProperties.limits.minUniformBufferOffsetAlignment);
	printf("deviceProperties.limits.maxPushConstantsSize: %i\n", deviceProperties.limits.maxPushConstantsSize);
	printf("deviceProperties.limits.maxMemoryAllocationCount: %i\n", deviceProperties.limits.maxMemoryAllocationCount);

	// Information about what the device can do (get shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(m_Device->getPhysicalDevice(), &deviceFeatures);

	QueueFamilyIndices indices = m_Device->findPhysicalQueueFamilies(); // getQueueFamilies();

	bool extensionsSupported = checkDeviceExtensionSupport();

	bool swapChainValid = false;
	if (extensionsSupported)
	{
		SwapChainVCA::SwapChainDetails swapChainDetails = m_SwapChain->getSwapChainDetails();
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	return indices.isValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}

/****
QueueFamilyIndices VulkanRenderer::getQueueFamilies()
{
	QueueFamilyIndices indices;

	// Get all Queue Family Property info for the given device
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_Device->getPhysicalDevice(), &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_Device->getPhysicalDevice(), &queueFamilyCount, queueFamilyList.data());

	// Go through each queue family and check if it has at least 1 of the required types of queue
	int i = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		// First check if queue family has at least 1 queue in that family (could have no queues)
		// Queue can be multiple types defined through bitfield.
		// Need to bitwise AND with VK_QUEUE_*_BIT to check if has required type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i; // If queue family is valid, then get index
		}

		// Check if Queue Family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_Device->getPhysicalDevice(), i, m_Device->surface(), &presentationSupport);
		// Check if queue is presentation type (can be both graphics and presentation)
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
		}

		// Check if queue family indices are in a valid state, stop searching if so
		if (indices.isValid())
		{
			break;
		}

		i++;
	}

	return indices;
}
****/

/****
SwapChainLVE::SwapChainDetails VulkanRenderer::getSwapChainDetails()
{
	SwapChainLVE::SwapChainDetails swapChainDetails;

	// -- SURFACE CAPABILITIES --
	// Get the surface capabilities for the given surface on the given physical device

	auto phyDevice = m_Device->getPhysicalDevice();
	auto surface = m_Device->surface();

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &swapChainDetails.surfaceCapabilities);

	// -- FORMATS --
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &formatCount, nullptr);

	// If formats return, get list of formats
	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &formatCount, swapChainDetails.formats.data());
	}

	// -- PRESENTATION MODES --
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &presentationCount, nullptr);

	// If presentation modes returned, get list of presentation modes
	if (presentationCount != 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &presentationCount, swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}
****/

/****
// Best format is subjective, but ours will be:
// Format:     VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
// colorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// If restricted, search for optimal format
	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && 
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	// If can't find optimal format, then just return first format
	return formats[0];
}
****/

/****
VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	// Look for VK_PRESENT_MODE_MAILBOX_KHR
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	// If can't find, use FIFO as Vulkan spec says it must be present
	return VK_PRESENT_MODE_FIFO_KHR;
}
****/

/****
VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		// If value can very, need to set manually

		// Get window size
		int width, height;
		glfwGetFramebufferSize(m_Window->getHandle(), &width, &height);

		// Create new extent using window size
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// Surface also defines max and min, so make sure within boundaries by clamping value
		newExtent.width = std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width);
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, newExtent.width);
		newExtent.height = std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height);
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, newExtent.height);

		return newExtent;
	}
}
****/

VkFormat VulkanRendererNew::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through options and find compatible one
	for (VkFormat format : formats)
	{
		// Get properties for given format on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_Device->getPhysicalDevice(), format, &properties);

		// Depending on tiling choice, need to check for different bit flag
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find a matching format!");
}

VkImage VulkanRendererNew::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
	// CREATE IMAGE
	// Image Creation Info
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;              // Type of image (1D, 2D or 3D)
	imageCreateInfo.extent.width = width;                      // Width of image extent
	imageCreateInfo.extent.height = height;                    // Height of image extent
	imageCreateInfo.extent.depth = 1;                          // Depth of image (just 1, no 3D aspect)
	imageCreateInfo.mipLevels = 1;                             // Number of mipmap levels
	imageCreateInfo.arrayLayers = 1;                           // Number of levels in image array
	imageCreateInfo.format = format;                           // Format type of image
	imageCreateInfo.tiling = tiling;                           // How image data should be "tiled" (arranged for optimal reading)
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout of image data on creation
	imageCreateInfo.usage = useFlags;                          // Bit flags defining what image will be used for
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;           // Number of samples for multi-sampling
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;   // Whether image can be shared between queues

	VkImage image;
	VkResult result = vkCreateImage(m_Device->device(), &imageCreateInfo, nullptr, &image);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image!");
	}

	printf("Vulkan Image successfully created.\n");

	// CREATE MEMORY FOR IMAGE

	// Get memory requirements for a type of image
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_Device->device(), image, &memoryRequirements);

	// Allocate memory using image requirements and user defined properties
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(m_Device->getPhysicalDevice(), memoryRequirements.memoryTypeBits, propFlags);

	result = vkAllocateMemory(m_Device->device(), &memoryAllocInfo, nullptr, imageMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate memory for image!");
	}

	printf("Vulkan Image Memory successfully allocated.\n");

	// Connect memory to image
	vkBindImageMemory(m_Device->device(), image, *imageMemory, 0);

	return image;
}

VkImageView VulkanRendererNew::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};

	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;                                // Image create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;             // Type if image (1D, 2D, 3D, Cube etc)
	viewCreateInfo.format = format;                              // Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows remapping of RGBA components to other RGBA values
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources - allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;    // Which aspect of image to view (e.g. COLOR_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;            // Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;              // Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;          // Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;              // Number of array levels to view

	// Create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(m_Device->device(), &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an ImageView!");
	}

	printf("Vulkan ImageView successfully created.\n");

	return imageView;
}

VkShaderModule VulkanRendererNew::createShaderModule(const std::vector<char>& code)
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

void VulkanRendererNew::createSynchronization()
{
	imageAvailable.resize(MAX_FRAME_DRAWS);
	renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);

	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		result = vkCreateSemaphore(m_Device->device(), &semaphoreCreateInfo, nullptr, &imageAvailable[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Semaphore ['imageAvailable']!");
		}
		printf("Vulkan Semaphore imageAvailable[%zu] successfully created.\n", i);

		result = vkCreateSemaphore(m_Device->device(), &semaphoreCreateInfo, nullptr, &renderFinished[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Semaphore ['renderFinished']!");
		}
		printf("Vulkan Semaphore renderFinished[%zu] successfully created.\n", i);

		result = vkCreateFence(m_Device->device(), &fenceCreateInfo, nullptr, &drawFences[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Fence!");
		}
		printf("Vulkan Fence drawFences[%zu] successfully created.\n", i);
	}
}

void VulkanRendererNew::createTextureSampler()
{
	// Sampler Creation Info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR; // How to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR; // How to render when image is minified on screen
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // How to handle texture weap in U (x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // How to handle texture weap in V (y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // How to handle texture weap in W (z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Border  beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;             // Whether coords should be normalized (between 0 and 1)
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;     // Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;                              // Level of Details bias for mip level
	samplerCreateInfo.minLod = 0.0f;                                  // Minimum Level of Detail to pick mip level
	samplerCreateInfo.maxLod = 0.0f;                                  // Maximum Level of Detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_TRUE;                     // Enable Anisotropy
	samplerCreateInfo.maxAnisotropy = 16;                             // Anisotropy sample level
	
	VkResult result = vkCreateSampler(m_Device->device(), &samplerCreateInfo, nullptr, &textureSampler);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Texture Sampler!");
	}

	printf("Vulkan Texture Sampler successfully created.\n");
}

int VulkanRendererNew::createTextureImage(std::string fileName)
{
	// Load image file
	int width, height;
	VkDeviceSize imageSize;
	stbi_uc* imageData = loadTextureFile(fileName, &width, &height, &imageSize);

	// Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;
	createBuffer(m_Device->getPhysicalDevice(), m_Device->device(), imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer, &imageStagingBufferMemory);

	// Copy image data to staging buffer
	void* data; // Creating a pointer to a random place in RAM
	vkMapMemory(m_Device->device(), imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imageData, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_Device->device(), imageStagingBufferMemory);

	// Free original image data
	stbi_image_free(imageData);

	// Create image to hold final texture
	VkImage texImage;
	VkDeviceMemory texImageMemory;
	texImage = createImage(width, height, VK_FORMAT_B8G8R8A8_UNORM, // VK_FORMAT_R8G8B8A8_UNORM
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&texImageMemory);

	// COPY DATA TO IMAGE

	// Transition image to be DST for copy operation
	// Use Memory Barrier to transition image layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	transitionImageLayout(m_Device->device(), m_Device->graphicsQueue(), graphicsCommandPool,
		texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy image data
	copyImageBuffer(m_Device->device(), m_Device->graphicsQueue(), graphicsCommandPool, imageStagingBuffer, texImage, width, height);

	// Transition image to be shader readable for shader usage
	transitionImageLayout(m_Device->device(), m_Device->graphicsQueue(), graphicsCommandPool,
		texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Add texture data to vector for reference
	textureImages.push_back(texImage);
	textureImageMemory.push_back(texImageMemory);

	vkDestroyBuffer(m_Device->device(), imageStagingBuffer, nullptr);
	vkFreeMemory(m_Device->device(), imageStagingBufferMemory, nullptr);

	// Return index of new texture image
	return (int)textureImages.size() - 1;
}

int VulkanRendererNew::createTexture(std::string fileName)
{
	// Create Texture Image and get its location in array
	int textureImageLoc = createTextureImage(fileName);

	// Create Image View and add to list
	VkImageView imageView = createImageView(textureImages[textureImageLoc], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT); // VK_FORMAT_R8G8B8A8_UNORM
	textureImageViews.push_back(imageView);

	// Create Texture Descriptor
	int descriptorLoc = createTextureDescriptor(imageView);

	// Return location of set with texture
	return descriptorLoc;
}

int VulkanRendererNew::createTextureDescriptor(VkImageView textureImage)
{
	VkDescriptorSet descriptorSet;

	// Descriptor Set Allocation Info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = samplerDescriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &samplerSetLayout;

	// Allocate Descriptor Sets
	VkResult result = vkAllocateDescriptorSets(m_Device->device(), &setAllocInfo, &descriptorSet);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Texture Descriptor Sets!");
	}

	printf("Vulkan Descriptor Sets (Texture Samplers) successfully allocated from the Descriptor Pool.\n");

	// Texture Image Info
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Image layout when in use
	imageInfo.imageView = textureImage;                               // Image to bind to set
	imageInfo.sampler = textureSampler;                               // Sampler to use for set

	// Descriptor Write Info
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	// Update new descriptor set
	vkUpdateDescriptorSets(m_Device->device(), 1, &descriptorWrite, 0, nullptr);

	// Add descriptor set to list
	samplerDescriptorSets.push_back(descriptorSet);

	// Return Descriptor Set location
	return (int)samplerDescriptorSets.size() - 1;
}

int VulkanRendererNew::createMeshModel(std::string modelFile)
{
	// Import model "scene"
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelFile,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices);

	if (!scene)
	{
		throw std::runtime_error("Failed to load model! (" + modelFile + ")");
	}

	// Get vector of all materials with 1:1 ID placement
	std::vector<std::string> textureNames = MeshModel::LoadMaterials(scene);

	// Conversion from the materials list IDs to our Descriptor Array IDs
	std::vector<int> matToTex(textureNames.size());

	// Loop over textureNames and create textures for them
	for (size_t i = 0; i < textureNames.size(); i++)
	{
		// If material had no texture, set '0' to indicate no texture, 
		// texture 0 will be reserved for a default texture
		if (textureNames[i].empty())
		{
			matToTex[i] = 0;
		}
		else
		{
			// Otherwise, create texture and set value to index of new texture
			matToTex[i] = createTexture(textureNames[i]);
		}
	}

	// Load in all our meshes
	std::vector<Mesh> modelMeshes = MeshModel::LoadNode(m_Device->getPhysicalDevice(), m_Device->device(),
		m_Device->graphicsQueue(), graphicsCommandPool, scene->mRootNode, scene, matToTex);

	// Create mesh model and add to list
	MeshModel meshModel = MeshModel(modelMeshes);
	modelList.push_back(meshModel);

	return (int)modelList.size() - 1;
}

stbi_uc* VulkanRendererNew::loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
{
	// Number of channels the image uses
	int channels;

	// Load pixel data for image
	std::string fileLoc = "Textures/" + fileName;
	stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

	if (!image)
	{
		throw std::runtime_error("Failed to load a Texture file! (" + fileName + ")");
	}

	// Calculate image size using given and known data
	*imageSize = (VkDeviceSize)(*width * *height * 4);

	printf("Texture file '%s' successfully loaded.\n", fileLoc.c_str());

	return image;
}
