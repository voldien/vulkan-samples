#ifndef _COMMON_VULKAN_CORE_H_
#define _COMMON_VULKAN_CORE_H_ 1
#include <vector>
#include<string>
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

class VulkanCore {
  public:
	VulkanCore(int argc, const char **argv);
	VulkanCore(const VulkanCore &other) = delete;
	~VulkanCore(void);

	virtual void Initialize(void);

  public:

	/*	*/
	VkInstance inst;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkDebugReportCallbackEXT debugReport;
	VkExtensionProperties *extension_names;
	std::vector<VkExtensionProperties> instanceExtensions;
	/*  Physical device.    */
	VkPhysicalDevice gpu;

	VkPhysicalDeviceProperties gpu_props;
	VkQueueFamilyProperties *queue_props;
	uint32_t graphics_queue_node_index;

	bool enableValidationLayers;
	bool enableDebugTracer;

	uint32_t queue_count;
	std::vector<VkPhysicalDevice> physicalDevices;
	VkPhysicalDevice *physical_devices;

	VkDevice device;
	VkPhysicalDeviceMemoryProperties memProperties;

	VkQueue graphicsQueue; // TODO rename graphicsQueue
	VkQueue presentQueue;
};

#endif
