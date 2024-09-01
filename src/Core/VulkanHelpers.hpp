#pragma once

#include <vulkan/vulkan.h>

namespace vlknh {

	struct BufferCreateInfo {

		VkPhysicalDevice      physicalDevice;
		VkDeviceSize          size;
		VkBufferUsageFlags    usage;
		VkMemoryPropertyFlags properties;

	};

	VkShaderModule createShaderModule(VkDevice device, const char* fileName);
	void           createBuffer(VkDevice device, const BufferCreateInfo& pCreateInfo, VkBuffer* buff, VkDeviceMemory* buffMem);
	void           loadBuffer(VkDevice device, VkDeviceMemory buffMem, void* buffData, VkDeviceSize buffSize);

	void loadBuffer(VkDevice device, VkDeviceMemory buffMem, void* buffData, VkDeviceSize buffSize);

	class SingleTimeCommandBuffer {
	public:

		SingleTimeCommandBuffer(VkDevice device, VkCommandPool commandPool);

		void begin();
		void submit(VkQueue graphicsQueue);
		void copy(VkDeviceSize buffSize, VkBuffer src, VkBuffer dst) {
			VkBufferCopy copyRegion = { 0, 0, buffSize };
			vkCmdCopyBuffer(m_cmdBuff, src, dst, 1, &copyRegion);
		}

		~SingleTimeCommandBuffer() { vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_cmdBuff); }

	private:

			VkDevice        m_device;
			VkCommandPool   m_commandPool;
			VkCommandBuffer m_cmdBuff;

	};

}