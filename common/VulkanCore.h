#ifndef _COMMON_VULKAN_SAMPLES_CORE_H_
#define _COMMON_VULKAN_SAMPLES_CORE_H_ 1
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class PhysicalDevice;
/**
 * @brief
 *
 */
class VulkanCore {
	friend class VKWindow;

  public:
	// TODO remove argc and argv
	VulkanCore(int argc, const char **argv, const std::unordered_map<const char *, bool> &requested_extensions = {},
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

	/**
	 * @brief
	 *
	 * @param extension
	 * @return true
	 * @return false
	 */
	bool isInstanceExtensionSupported(const std::string &extension) const {
		return std::find_if(getInstanceExtensions().begin(), getInstanceExtensions().end(),
							[extension](const VkExtensionProperties &device_extension) {
								return std::strcmp(device_extension.extensionName, extension.c_str()) == 0;
							}) != getInstanceExtensions().cend();
	}

	/**
	 * @brief
	 *
	 * @param extension
	 * @return true
	 * @return false
	 */
	bool isInstanceLayerSupported(const std::string &extension) const {
		return std::find_if(getInstanceLayers().begin(), getInstanceLayers().end(),
							[extension](const VkLayerProperties &device_layers) {
								return std::strcmp(device_layers.layerName, extension.c_str()) == 0;
							}) != getInstanceLayers().cend();
	}

	/**
	 * @brief Get the Physical Devices object
	 *
	 * @return const std::vector<VkPhysicalDevice>&
	 */
	const std::vector<VkPhysicalDevice> &getPhysicalDevices(void) const noexcept { return this->physicalDevices; }

	uint32_t getNrPhysicalDevices(void) const noexcept { return getPhysicalDevices().size(); }

	/**
	 * @brief Get the Handle object
	 *
	 * @return VkInstance
	 */
	virtual VkInstance getHandle(void) const noexcept { return this->inst; }

	/**
	 * @brief Get the Device Group Properties object
	 *
	 * @return std::vector<VkPhysicalDeviceGroupProperties>
	 */
	std::vector<VkPhysicalDeviceGroupProperties> getDeviceGroupProperties(void) const noexcept {

		uint32_t nrGroups;
		vkEnumeratePhysicalDeviceGroups(this->getHandle(), &nrGroups, nullptr);
		std::vector<VkPhysicalDeviceGroupProperties> prop(nrGroups);
		if (nrGroups > 0)
			vkEnumeratePhysicalDeviceGroups(this->getHandle(), &nrGroups, prop.data());
		return prop;
	}

	/**
	 * @brief Create a Physical Devices object
	 *
	 * @return std::vector<PhysicalDevice *>
	 */
	std::vector<std::shared_ptr<PhysicalDevice>> createPhysicalDevices(void) const;

	/**
	 * @brief Create a Physical Device object
	 *
	 * @param index
	 * @return PhysicalDevice*
	 */
	std::shared_ptr<PhysicalDevice> createPhysicalDevice(unsigned int index) const;

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
