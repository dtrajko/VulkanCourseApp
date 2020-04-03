#include "VulkanRenderer.h"

#include <stdexcept>


VulkanRenderer::VulkanRenderer()
{

}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	window = newWindow;

	try
	{
		createInstance();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::cleanup()
{
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::createInstance()
{
	// Information about the application itself
	// Most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";                // Custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // Custom version of the application
	appInfo.pEngineName = "No Engine";                      // Custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);       // Custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_1;                // The Vulkan API version

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

	// Check Instance Extensions supported
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("vkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO: Set up Validation Layers that Instance will use
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	// Create Instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance!");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// Get the Queue Family indices for the chosen Physical Device
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// Queue the logical device needs to create and info to do so (Only 1 for now, will add more later!)
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;                             // Number of queues to create
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;  // The index of the queue family to create a queue from
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority; // Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)

	// Information to create logical device (sometimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;              // Number of Queue Create Infos
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;  // List of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = 0;             // Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;     // List of enabled logical device extensions

	// Physical Device Features the Logical Device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;     // Physical Device features the Logical Device will use

	// Create the logical device for the given physical device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Logical Device!");
	}

	// Queues are created at the same time as the device
	// so we want handle to queues
	// From given logical device, of given Queue Family, of given Queue Index (0 since only one queue),
	// place reference in given VkQueue
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}

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
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
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
			if (strcmp(checkExtension, extension.extensionName))
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

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*
	// Information about the device itself (ID, name, type, verdor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (get shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	QueueFamilyIndices indices = getQueueFamilies(device);

	return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// Get all Queue Family Property info for the given device
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

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

		// Check if queue family indices are in a valid state, stop searching if so
		if (indices.isValid())
		{
			break;
		}

		i++;
	}

	return indices;
}
