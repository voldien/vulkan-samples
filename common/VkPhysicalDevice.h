#ifndef _VULKAN_COMMON_PHYSICAL_DEVICE_H_
#define _VULKAN_COMMON_PHYSICAL_DEVICE_H_ 1
#include "VulkanCore.h"

class PhysicalDevice {
  public:
	PhysicalDevice(const VulkanCore &core, VkPhysicalDevice device);
	PhysicalDevice(VkInstance instance, VkPhysicalDevice device);
	PhysicalDevice(const PhysicalDevice &) = delete;
	PhysicalDevice(PhysicalDevice &&) = delete;

	const VkPhysicalDeviceFeatures &getFeatures(void) const noexcept { return features; }

	const VkPhysicalDeviceProperties getProperties(void) const noexcept { return properties; }

	const VkPhysicalDeviceMemoryProperties getMemoryProperties(void) const noexcept { return memProperties; }

	const std::vector<VkQueueFamilyProperties> &getQueueFamilyProperties(void) const noexcept {
		return queueFamilyProperties;
	}

	bool isPresentable(VkSurfaceKHR surface, uint32_t queueFamilyIndex) const;

	VkPhysicalDevice getHandle(void) const noexcept { return this->mdevice; }

	// TODO add extensions

	// TODO add query of features.

	template <typename T> void checkFeature(VkStructureType type, T &requestFeature) {

		VkPhysicalDeviceFeatures2 feature = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
											 .pNext = &requestFeature};
		vkGetPhysicalDeviceFeatures2(getHandle(), &feature);
	}

	const char *getDeviceName(void) const noexcept { return this->properties.deviceName; }

  private:
	VkPhysicalDevice mdevice;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties properties;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
};

#endif