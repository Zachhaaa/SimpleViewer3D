#include "VulkanHelpers.hpp"

#include <cstdio> 
#include <cassert>
#include <memory>

VkShaderModule vlknh::createShaderModule(VkDevice device, const char* fileName) {

	FILE* file = fopen(fileName, "rb");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	std::unique_ptr<char[]> byteCode(new char[fileSize]);

	fread(byteCode.get(), sizeof(char), fileSize, file);

	fclose(file);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = (uint32_t*)byteCode.get();

	VkShaderModule shaderModule;
	VkResult err = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	assert(err == VK_SUCCESS && "Shader Module Creation failed");

	return shaderModule;

}
void vlknh::createBuffer(VkDevice device, const BufferCreateInfo& pCreateInfo, VkBuffer* buff, VkDeviceMemory* buffMem) {

	VkBufferCreateInfo buffInfo{};
	buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffInfo.size = pCreateInfo.size;
	buffInfo.usage = pCreateInfo.usage;
	buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult err = vkCreateBuffer(device, &buffInfo, nullptr, buff);
	assert(err == VK_SUCCESS && "Buffer creation failed");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buff, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(pCreateInfo.physicalDevice, &memProps);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;

	uint32_t i = 0;
	for (; i < memProps.memoryTypeCount; i++) {
		if (memRequirements.memoryTypeBits & (1 << i) && memProps.memoryTypes[i].propertyFlags & pCreateInfo.properties) {
			break;
		}
	}
	allocInfo.memoryTypeIndex = i;

	err = vkAllocateMemory(device, &allocInfo, nullptr, buffMem);
	assert(err == VK_SUCCESS && "Buffer allocation failed");

	vkBindBufferMemory(device, *buff, *buffMem, 0);

}
void vlknh::loadBuffer(VkDevice device, VkDeviceMemory buffMem, void* buffData, VkDeviceSize buffSize) {
	void* data;
	vkMapMemory(device, buffMem, 0, buffSize, 0, &data);
	memcpy(data, buffData, buffSize);
	vkUnmapMemory(device, buffMem);
}
vlknh::SingleTimeCommandBuffer::SingleTimeCommandBuffer(VkDevice device, VkCommandPool commandPool)
	: m_device(device), m_commandPool(commandPool)
{

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_device, &allocInfo, &m_cmdBuff);

}
void vlknh::SingleTimeCommandBuffer::begin() {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_cmdBuff, &beginInfo);
}
void vlknh::SingleTimeCommandBuffer::submit(VkQueue graphicsQueue) {

	vkEndCommandBuffer(m_cmdBuff);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_cmdBuff;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

}