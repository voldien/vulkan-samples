#ifndef _VKSAMPLES_VK_HELPER_H_
#define _VKSAMPLES_VK_HELPER_H_ 1
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define ArraySize(a) (sizeof(a) / sizeof(*a))

class VKHelper {
  public:
	/*  Helper functions.   */
	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
								   VkMemoryPropertyFlags properties);

	static uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties *memProperties, uint32_t typeFilter,
							VkMemoryPropertyFlags properties);

	static void createBuffer(VkDevice device, VkDeviceSize size, VkPhysicalDeviceMemoryProperties *memoryProperies,
							 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
							 VkDeviceMemory &bufferMemory);

	// static VkImageView createImageView(VulkanCore *vulkanCore, VkImage image, VkFormat format);

	static VkShaderModule createShaderModule(VkDevice device, std::vector<char> &data);

	//
	// static bool isDeviceSuitable(VkPhysicalDevice device);

	static void selectDefaultDevices(std::vector<VkPhysicalDevice> &devices,
									 std::vector<VkPhysicalDevice> &selectDevices,
									 uint32_t device_type_filter = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU |
																   VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

	// TODO improve to accomudate the configurations.
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes, bool vsync);

	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D actualExtent);

	struct QueueFamilyIndices {
		uint32_t graphicsFamily = -1;
		uint32_t presentFamily = -1;

		bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
	};

	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
};

#define CHECK_VK_ERROR(result)

#endif
