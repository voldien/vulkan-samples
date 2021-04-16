#ifndef _VULKAN_COMMON_PHYSICAL_DEVICE_H_
#define _VULKAN_COMMON_PHYSICAL_DEVICE_H_ 1
#include "VulkanCore.h"

/**
 * @brief
 *
 */
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

	/**
	 * @brief
	 *
	 * @param surface
	 * @param queueFamilyIndex
	 * @return true
	 * @return false
	 */
	bool isPresentable(VkSurfaceKHR surface, uint32_t queueFamilyIndex) const;

	VkPhysicalDevice getHandle(void) const noexcept { return this->mdevice; }

	/**
	 * @brief Get the Extensions object
	 *
	 * @return const std::vector<VkExtensionProperties>&
	 */
	const std::vector<VkExtensionProperties> &getExtensions(void) const noexcept { return this->extensions; }

	/**
	 * @brief
	 *
	 * @param extension
	 * @return true
	 * @return false
	 */
	bool isExtensionSupported(const std::string &extension) const {
		return std::find_if(getExtensions().begin(), getExtensions().end(),
							[extension](const VkExtensionProperties &device_extension) {
								return std::strcmp(device_extension.extensionName, extension.c_str()) == 0;
							}) != getExtensions().cend();
	}

	/**
	 * @brief
	 *
	 * @tparam T
	 * @param type
	 * @param requestFeature
	 */
	template <typename T> void checkFeature(VkStructureType type, T &requestFeature) {

		VkPhysicalDeviceFeatures2 feature = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
											 .pNext = &requestFeature};
		requestFeature.sType = type;
		vkGetPhysicalDeviceFeatures2(getHandle(), &feature);
	}

	template <typename T> void getProperties(VkStructureType type, T &requestProperties) {
		VkPhysicalDeviceProperties2 properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &requestProperties
		};
		requestProperties.sType = type;
		vkGetPhysicalDeviceProperties2(getHandle(), &properties);
	}

	const char *getDeviceName(void) const noexcept { return this->properties.deviceName; }

  private:
	VkPhysicalDevice mdevice;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties properties;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	std::vector<VkExtensionProperties> extensions;
};

#endif
