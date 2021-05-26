#include "VKDevice.h"

VKDevice::VKDevice(const std::vector<std::shared_ptr<PhysicalDevice>> &devices,
				   const std::unordered_map<const char *, bool> &requested_extensions, VkQueueFlags requiredQueues) {

	std::vector<VkPhysicalDevice> selectedDevices;
	// VKHelper::selectDefaultDevices(physicalDevices, selectedDevices);

	/*	*/
	// this->gpu = selectedDevices[0];

	// vkEnumerateDeviceLayerPropertie

	/*  Select queue with graphic.  */ // TODO add queue for compute and etc.
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t computeQueueNodeIndex;

	int nrQueues = 1;
	for (uint32_t i = 0; i < devices[0]->getQueueFamilyProperties().size(); i++) {
		/*  */
		const VkQueueFamilyProperties &prop = devices[0]->getQueueFamilyProperties()[i];

		/*  */
		if ((prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && (requiredQueues & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (graphicsQueueNodeIndex == UINT32_MAX)
				graphicsQueueNodeIndex = i;
		}
		if ((prop.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
			if (computeQueueNodeIndex == UINT32_MAX)
				computeQueueNodeIndex = i;
		}
	}

	// TODO resolve in case compute is not supported on the graphic queue.
	this->graphics_queue_node_index = graphicsQueueNodeIndex;
	this->compute_queue_node_index = graphicsQueueNodeIndex;

	std::vector<VkDeviceQueueCreateInfo> queueCreations;
	std::vector<float> queuePriorities;
	float queue_priorities[1] = {1.0};
	const VkDeviceQueueCreateInfo queue = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										   .pNext = NULL,
										   .flags = 0,
										   .queueFamilyIndex = this->graphics_queue_node_index,
										   .queueCount = nrQueues,
										   .pQueuePriorities = queue_priorities};

	/*  Required extensions.    */
	std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME, /*	*/
	};

	/*	Iterate through each extension and add if supported.	*/
	for (const std::pair<const char *, bool> &n : requested_extensions) {
		if (n.second) {
			if (devices[0]->isExtensionSupported(n.first))
				deviceExtensions.push_back(n.first);
			else
				throw std::runtime_error(
					fmt::format("{} does not support: {}\n", devices[0]->getDeviceName(), n.first));
		}
	}

	VkDeviceGroupDeviceCreateInfo deviceGroupDeviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
	};
	if (devices.size() > 0) {
		// if (phyiscalGroupDevices[0].physicalDeviceCount > 1) {
		// 	deviceGroupDeviceCreateInfo.physicalDeviceCount = phyiscalGroupDevices[0].physicalDeviceCount;
		// 	deviceGroupDeviceCreateInfo.pPhysicalDevices = phyiscalGroupDevices[0].physicalDevices;
		// }
	}

	VkDeviceCreateInfo device = {};
	device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device.pNext = VK_NULL_HANDLE;
	device.queueCreateInfoCount = 1;
	device.pQueueCreateInfos = &queue;

	// /*	Enable group if supported.	*/
	// if (deviceGroupDeviceCreateInfo.physicalDeviceCount > 1) {
	// 	deviceShaderDrawParametersFeatures.pNext = &deviceGroupDeviceCreateInfo;
	// }
	device.enabledExtensionCount = deviceExtensions.size();
	device.ppEnabledExtensionNames = deviceExtensions.data();

	/*  Create device.  */
	vkCreateDevice(devices[0]->getHandle(), &device, VK_NULL_HANDLE, &this->logicalDevice);

	/*  Get all queues.    */
	vkGetDeviceQueue(getHandle(), this->graphics_queue_node_index, 0, &this->graphicsQueue);
	vkGetDeviceQueue(getHandle(), this->graphics_queue_node_index, 0, &this->presentQueue);
	vkGetDeviceQueue(getHandle(), this->compute_queue_node_index, 0, &this->computeQueue);

	this->physicalDevices = devices;
}

VKDevice::~VKDevice(void) {
	if (getHandle() != VK_NULL_HANDLE)
		vkDestroyDevice(getHandle(), VK_NULL_HANDLE);
}