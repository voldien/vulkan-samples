#ifndef _VULKAN_COMMON_PHYSICAL_DEVICE_H_
#define _VULKAN_COMMON_PHYSICAL_DEVICE_H_ 1
#include "VulkanCore.h"
#include <stdexcept>

/**
 * @brief
 *
 */
class PhysicalDevice {
  public:
	PhysicalDevice(const std::shared_ptr<VulkanCore> &core, VkPhysicalDevice device);
	PhysicalDevice(VkInstance instance, VkPhysicalDevice device);
	PhysicalDevice(const PhysicalDevice &) = delete;
	PhysicalDevice(PhysicalDevice &&) = delete;

	const VkPhysicalDeviceFeatures &getFeatures(void) const noexcept { return features; }

	VkPhysicalDeviceProperties getProperties(void) noexcept { return properties; }
	const VkPhysicalDeviceProperties &getProperties(void) const noexcept { return properties; }

	VkPhysicalDeviceMemoryProperties getMemoryProperties(void) noexcept { return memProperties; }
	const VkPhysicalDeviceMemoryProperties &getMemoryProperties(void) const noexcept { return memProperties; }

	const VkPhysicalDeviceLimits &getDeviceLimits(void) const noexcept { return this->properties.limits; }

	const VkPhysicalDeviceDriverProperties &getDeviceDriverProperties(void) {
		VkPhysicalDeviceDriverProperties devceProp;
		getProperties(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES, devceProp);
	}

	const VkPhysicalDeviceSubgroupProperties getDeviceSubGroupProperties(void) {
		VkPhysicalDeviceSubgroupProperties devceProp;
		getProperties(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES, devceProp);
	}

	/**
	 * @brief Get the Queue Family Properties object
	 * Get all the support family properties.
	 *
	 * @return const std::vector<VkQueueFamilyProperties>&
	 */
	const std::vector<VkQueueFamilyProperties> &getQueueFamilyProperties(void) const noexcept {
		return queueFamilyProperties;
	}

	bool isQueueSupported(VkQueueFlags queueFlag) const {
		for (const VkQueueFamilyProperties &a : getQueueFamilyProperties()) {
			if (a.queueFlags & queueFlag)
				return true;
		}
		return false;
	}

	/**
	 * @brief
	 *
	 * @param surface
	 * @param queueFamilyIndex
	 * @return true
	 * @return false
	 */
	bool isPresentable(VkSurfaceKHR surface, uint32_t queueFamilyIndex) const noexcept;

	/**
	 * @brief
	 *
	 * @param format
	 * @param imageType
	 * @param tiling
	 * @param usage
	 * @return true
	 * @return false
	 */
	bool isFormatSupported(VkFormat format, VkImageType imageType, VkImageTiling tiling,
						   VkImageUsageFlags usage) const {
		VkImageFormatProperties prop;
		VkResult result =
			vkGetPhysicalDeviceImageFormatProperties(this->getHandle(), format, imageType, tiling, usage, 0, &prop);
		if (result == VK_SUCCESS)
			return true;
		else if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
			return false;
		else
			VKS_VALIDATE(result);
	}

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
	bool isExtensionSupported(const std::string &extension) const noexcept {
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
	template <typename T> void checkFeature(VkStructureType type, T &requestFeature) noexcept {

		VkPhysicalDeviceFeatures2 feature = {};
		feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		feature.pNext = &requestFeature;

		requestFeature.sType = type;
		vkGetPhysicalDeviceFeatures2(getHandle(), &feature);
	}

	/**
	 * @brief Get the Properties object
	 *
	 * @tparam T
	 * @param type
	 * @param requestProperties
	 */
	template <typename T> void getProperties(VkStructureType type, T &requestProperties) noexcept {
		VkPhysicalDeviceProperties2 properties{};
		properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		properties.pNext = &requestProperties;
		/*	*/
		requestProperties.sType = type;
		vkGetPhysicalDeviceProperties2(getHandle(), &properties);
	}

	const char *getDeviceName(void) const noexcept { return this->properties.deviceName; }

  private:
	VkPhysicalDevice mdevice;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceLimits limits;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	std::vector<VkExtensionProperties> extensions;
	std::shared_ptr<VulkanCore> vkCore;
};

#endif
