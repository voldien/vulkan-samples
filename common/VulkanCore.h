#ifndef _COMMON_VULKAN_CORE_H_
#define _COMMON_VULKAN_CORE_H_ 1
#include <string>
#include <vector>
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

class VulkanCore {
	friend class VKWindow;

  public:
	VulkanCore(int argc, const char **argv, const std::vector<const char *> &layers = {});
	VulkanCore(const VulkanCore &other) = delete;
	VulkanCore(VulkanCore &&other) = delete;
	~VulkanCore(void);

	VulkanCore &operator=(const VulkanCore &) = delete;
	VulkanCore &operator=(VulkanCore &&) = delete;

	virtual void Initialize(const std::vector<const char *> &layers);

	const std::vector<VkExtensionProperties> &getInstanceExtensions(void) const noexcept { return this->instanceExtensions; }
	const std::vector<VkPhysicalDevice> &getPhysicalDevices(void) const noexcept { return this->physicalDevices; }
	virtual VkInstance getHandle(void) const noexcept { return this->inst; }

  private:
	void parseOptions(int argc, const char **argv);

  protected:
	/*	*/
	std::vector<VkExtensionProperties> instanceExtensions;
	VkInstance inst;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkDebugReportCallbackEXT debugReport;

	/*  Physical device.    */
	VkPhysicalDevice gpu;

	VkPhysicalDeviceProperties gpu_props;
	VkQueueFamilyProperties *queue_props;
	uint32_t graphics_queue_node_index;
	uint32_t compute_queue_node_index;

	bool enableValidationLayers;
	bool enableDebugTracer;

	uint32_t queue_count;
	std::vector<VkPhysicalDevice> physicalDevices;
	// std::vector<VkPhysicalDeviceMemoryProperties> memProper;

	VkDevice device;
	VkPhysicalDeviceMemoryProperties memProperties;

	// TODO
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
};

#endif
