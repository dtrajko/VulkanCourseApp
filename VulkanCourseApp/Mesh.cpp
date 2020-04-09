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
	createVertexBuffer(vertices);
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

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();
	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkMemoryPropertyFlags bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	createBuffer(physicalDevice, device, bufferSize, bufferUsage, bufferProperties, &vertexBuffer, &vertexBufferMemory);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;                                                       // 1. Create pointer to a point in normal memory
	vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);               // 3. Copy memory from vertices vector to the point
	vkUnmapMemory(device, vertexBufferMemory);                        // 4. Unmap the vertex buffer memory
}
