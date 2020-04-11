#include "Mesh.h"

Mesh::Mesh()
{
	vertexCount = 0;
	physicalDevice = nullptr;
	device = nullptr;
	vertexBuffer = nullptr;
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
	VkQueue transferQueue, VkCommandPool transferCommandPool,
	std::vector<Vertex>* vertices, std::vector<uint32_t>* indices)
{
	vertexCount = (int)vertices->size();
	indexCount = (int)indices->size();
	physicalDevice = newPhysicalDevice;
	device = newDevice;
	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

	model.model = glm::mat4(1.0f);
}

void Mesh::setModel(glm::mat4 newModel)
{
	model.model = newModel;
}

Model Mesh::getModel()
{
	return model;
}

int Mesh::getVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

int Mesh::getIndexCount()
{
	return indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
	return indexBuffer;
}

void Mesh::destroyBuffers()
{
	// Destroy Vertex Buffer
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	// Destroy Index Buffer
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create Vertex Buffer and allocate memory to it
	VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags stagingBufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	createBuffer(physicalDevice, device, bufferSize, stagingBufferUsage, stagingBufferProperties, &stagingBuffer, &stagingBufferMemory);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;                                                        // 1. Create pointer to a point in normal memory
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);                // 3. Copy memory from vertices vector to the point
	vkUnmapMemory(device, stagingBufferMemory);                        // 4. Unmap the vertex buffer memory

	// Create a buffer with VK_BUFFER_USAGE_TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX BUFFER)
	// Buffer memory to be DEVICE_LOCAL_BIT meaning memory is on GPU and only accessible by it and not CPU (host)
	VkBufferUsageFlags dstBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkMemoryPropertyFlags dstBufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	createBuffer(physicalDevice, device, bufferSize, dstBufferUsage, dstBufferProperties, &vertexBuffer, &vertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

	// Clean up staging buffer parts
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	// Get size of buffer needed for indices
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to "stage" index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags stagingBufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	createBuffer(physicalDevice, device, bufferSize, stagingBufferUsage, stagingBufferProperties, &stagingBuffer, &stagingBufferMemory);

	// MAP MEMORY TO INDEX BUFFER
	void* data;                                                        // 1. Create pointer to a point in normal memory
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, indices->data(), (size_t)bufferSize);                 // 3. Copy memory from indices vector to the point
	vkUnmapMemory(device, stagingBufferMemory);                        // 4. Unmap the index buffer memory

	// Create buffer for INDEX data on GPU access only area
	VkBufferUsageFlags dstBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	VkMemoryPropertyFlags dstBufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	createBuffer(physicalDevice, device, bufferSize, dstBufferUsage, dstBufferProperties, &indexBuffer, &indexBufferMemory);

	// Copy from staging buffer to GPU access buffer
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

	// Destroy + release Staging Buffer resources
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
