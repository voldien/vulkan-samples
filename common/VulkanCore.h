#ifndef _COMMON_VULKAN_CORE_H_
#define _COMMON_VULKAN_CORE_H_ 1
#include <string>
#include <vector>
// #define VK_USE_PLATFORM_XLIB_KHR
// #define VK_USE_PLATFORM_WAYLAND_KHR
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vulkan/vulkan.h>

class PhysicalDevice;
/**
 * @brief
 *
 */
class VulkanCore {
	friend class VKWindow;

  public:
	VulkanCore(int argc, const char **argv,
			   const std::unordered_map<const char *, bool> &requested_extensions = {},
			   const std::unordered_map<const char *, bool> &requested_layers = {
				   {"VK_LAYER_KHRONOS_validation", true}});
	VulkanCore(const VulkanCore &other) = delete;
	VulkanCore(VulkanCore &&other) = delete;
	~VulkanCore(void);

	VulkanCore &operator=(const VulkanCore &) = delete;
	VulkanCore &operator=(VulkanCore &&) = delete;

	virtual void Initialize(const std::unordered_map<const char *, bool> &requested_extensions,
							const std::unordered_map<const char *, bool> &requested_layers);

	const std::vector<VkExtensionProperties> &getInstanceExtensions(void) const noexcept {
		return this->instanceExtensions;
	}
	const std::vector<VkLayerProperties> &getInstanceLayers(void) const noexcept { return this->instanceLayers; }

	bool isInstanceExtensionSupported(const std::string &extension) const {
		return std::find_if(getInstanceExtensions().begin(), getInstanceExtensions().end(),
							[extension](const VkExtensionProperties &device_extension) {
								return std::strcmp(device_extension.extensionName, extension.c_str()) == 0;
							}) != getInstanceExtensions().cend();
	}

	bool isInstanceLayerSupported(const std::string &extension) const {
		return std::find_if(getInstanceLayers().begin(), getInstanceLayers().end(),
							[extension](const VkLayerProperties &device_layers) {
								return std::strcmp(device_layers.layerName, extension.c_str()) == 0;
							}) != getInstanceLayers().cend();
	}

	const std::vector<VkPhysicalDevice> &getPhysicalDevices(void) const noexcept { return this->physicalDevices; }
	virtual VkInstance getHandle(void) const noexcept { return this->inst; }

	std::vector<VkPhysicalDeviceGroupProperties> getDeviceGroupProperties(void) const noexcept{

		uint32_t nrGroups;
		vkEnumeratePhysicalDeviceGroups(this->getHandle(), &nrGroups, nullptr);
		std::vector<VkPhysicalDeviceGroupProperties> prop(nrGroups);
		vkEnumeratePhysicalDeviceGroups(this->getHandle(), &nrGroups, prop.data());
		return prop;
	}
	int getNrGroupDevices(void) const noexcept { return this->nrGroupDevices; }
	// std::vector<VkPhysicalDevice> getGroupDevices(unsigned int index) const noexcept;

	std::vector<PhysicalDevice *> createPhysicalDevices(void) const;
	PhysicalDevice *createPhysicalDevice(unsigned int index) const;

  private:
	void parseOptions(int argc, const char **argv);

  protected:
	/*	*/
	std::vector<VkExtensionProperties> instanceExtensions;
	std::vector<VkLayerProperties> instanceLayers;
	VkInstance inst;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkDebugReportCallbackEXT debugReport;

	int nrGroupDevices;

	bool useValidationLayers;
	bool enableDebugTracer;

	uint32_t queue_count;
	std::vector<VkPhysicalDevice> physicalDevices;
};

#endif
