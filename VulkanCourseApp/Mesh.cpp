#include "Mesh.h"

Mesh::Mesh()
{
	vertexCount = 0;
	physicalDevice = nullptr;
	device = nullptr;
	vertexBuffer = nullptr;
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
	vertexCount = (int)vertices->size();
	physicalDevice = newPhysicalDevice;
	device = newDevice;
	vertexBuffer = createVertexBuffer(vertices);
}

int Mesh::getVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

void Mesh::destroyVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}

VkBuffer Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(Vertex) * vertices->size();  // Size of buffer (size of 1 vertex * number of vertices)
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Multiple types of buffer possible, we want Vertex Buffer
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;   // Similar to Swap Chain images, can share Vertex Buffers

	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vertex Buffer!");
	}

	printf("Vulkan Vertex Buffer successfully created.\n");

	// GET BUFFER MEMORY REQUIREMENTS
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, // Index of memory type on Physical Device that has required
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);      // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU can interact with memory
	                                                                                      // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping
	                                                                                      // (otherwise would have to specify manually)

	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &vertexBufferMemory);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	printf("Vulkan Vertex Buffer Memory successfully allocated.\n");

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;                                                            // 1. Create pointer to a point in normal memory
	vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferInfo.size);               // 3. Copy memory from vertices vector to the point
	vkUnmapMemory(device, vertexBufferMemory);                             // 4. Unmap the vertex buffer memory

	return vertexBuffer;
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((allowedTypes & (1 << i)) // Index of memory type must match corresponding bit in allowedTypes
			&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) // Desired property bit flags are part of memory type's bit flags
		{
			// This memory type is valid, so return its index
			return i;
		}
	}
}
