#ifndef _VK_SAMPLES_COMMON_VK_DEVICE_H_
#define _VK_SAMPLES_COMMON_VK_DEVICE_H_ 1
#include "VkPhysicalDevice.h"
#include "VulkanCore.h"
#include <unordered_map>
#include<fmt/core.h>

/**
 * @brief 
 * 
 */
class VKDevice {
  public:
	// TODO add support for group device.!
	/**
	 * @brief Construct a new VKDevice object
	 * 
	 * @param physicalDevices 
	 * @param requested_extensions 
	 * @param requiredQueues 
	 */
	VKDevice(const std::vector<PhysicalDevice *> &physicalDevices,
			 const std::unordered_map<const char *, bool>& requested_extensions = {},
			 VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);

	VKDevice(const PhysicalDevice *physicalDevice);
	VKDevice(const VKDevice &) = delete;
	VKDevice(VKDevice &&) = delete;
	~VKDevice(void);

	/**
	 * @brief
	 *
	 * @return true
	 * @return false
	 */
	bool isGroupDevice(void) const noexcept;
	const std::vector<PhysicalDevice *> &getPhysicalDevices(void) const noexcept { return mDevices; }

	VkDevice getHandle(void) const noexcept { return this->logicalDevice; }

	VkQueue getDefaultGraphicQueue(void) const noexcept { return this->graphicsQueue; }
	VkQueue getDefaultPresent(void) const noexcept { return this->presentQueue; }
	VkQueue getDefaultCompute(void) const noexcept { return this->computeQueue; }

	uint32_t getDefaultGraphicQueueIndex(void) const noexcept { return this->graphics_queue_node_index; }

	/**
	 * @brief 
	 * 
	 * @param typeFilter 
	 * @param properties 
	 * @return uint32_t 
	 */
	uint32_t findMemoryType(uint32_t typeFilter,
									VkMemoryPropertyFlags properties) const {

		/*	Iterate throw each memory types.	*/
		for (uint32_t i = 0; i < mDevices[0]->getMemoryProperties().memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (mDevices[0]->getMemoryProperties().memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error(fmt::format("failed to find suitable memory type {}!", typeFilter));
	}

	/**
	 * @brief 
	 * 
	 * @param format 
	 * @return true 
	 * @return false 
	 */
	bool isFormatedSupported(VkFormat format) const noexcept;

  private:
	uint32_t graphics_queue_node_index;
	uint32_t compute_queue_node_index;
	uint32_t transfer_queue_node_index;

	std::vector<PhysicalDevice *> mDevices;
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;
};

#endif
