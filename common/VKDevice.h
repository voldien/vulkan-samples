#ifndef _VK_SAMPLES_COMMON_VK_DEVICE_H_
#define _VK_SAMPLES_COMMON_VK_DEVICE_H_ 1
#include "VkPhysicalDevice.h"
#include "VulkanCore.h"
#include <unordered_map>

class VKDevice {
  public:
	// TODO add support for group device.!
	VKDevice(const std::vector<PhysicalDevice*> &devices,
			 std::unordered_map<const char *, bool> requested_extensions = {});
	VKDevice(const VKDevice &) = delete;
	VKDevice(VKDevice &&) = delete;

	bool isGroupDevice(void) const noexcept;
	const std::vector<PhysicalDevice*> &getPhysicalDevices(void) const noexcept { return mDevices; }

	VkDevice getHandle(void) const noexcept { return this->logicalDevice; }

	VkQueue getDefaultGraphicQueue(void) const noexcept { return this->graphicsQueue; }
	VkQueue getDefaultPresent(void) const noexcept { return this->presentQueue; }
	VkQueue getDefaultCompute(void) const noexcept { return this->computeQueue; }

    uint32_t getDefaultGraphicQueueIndex(void) const noexcept{return this->graphics_queue_node_index;}

  private:
	uint32_t graphics_queue_node_index;
	uint32_t compute_queue_node_index;
	std::vector<PhysicalDevice*> mDevices;
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
};

#endif
