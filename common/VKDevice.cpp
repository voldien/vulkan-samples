#include "VKDevice.h"

VKDevice::VKDevice(const std::vector<std::shared_ptr<PhysicalDevice>> &devices,
				   const std::unordered_map<const char *, bool> &requested_extensions, VkQueueFlags requiredQueues) {

	std::vector<VkPhysicalDevice> selectedDevices;
	// VKHelper::selectDefaultDevices(physicalDevices, selectedDevices);

	/*	*/
	// this->gpu = selectedDevices[0];

	// vkEnumerateDeviceLayerPropertie

	/*  Select queue with graphic.  */
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t computeQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	uint32_t transferQueueNodeIndex = UINT32_MAX;

	uint32_t nrQueues = 1;
	for (uint32_t i = 0; i < devices[0]->getQueueFamilyProperties().size(); i++) {
		/*  */
		const VkQueueFamilyProperties &familyProp = devices[0]->getQueueFamilyProperties()[i];

		/*  */
		if ((familyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && (requiredQueues & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (graphicsQueueNodeIndex == UINT32_MAX)
				graphicsQueueNodeIndex = i;
		}
		if ((familyProp.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && (requiredQueues & VK_QUEUE_COMPUTE_BIT) != 0) {
			if (computeQueueNodeIndex == UINT32_MAX)
				computeQueueNodeIndex = i;
		}
	}

	// TODO resolve in case compute is not supported on the graphic queue.
	if ((requiredQueues & VK_QUEUE_COMPUTE_BIT) && computeQueueNodeIndex == UINT32_MAX) {
	}
	this->graphics_queue_node_index = graphicsQueueNodeIndex;
	this->compute_queue_node_index = computeQueueNodeIndex;
	this->present_queue_node_index = graphics_queue_node_index;

	std::vector<VkDeviceQueueCreateInfo> queueCreations(nrQueues);
	std::vector<float> queuePriorities(nrQueues);
	float queue_priorities[1] = {1.0};
	if ((requiredQueues & VK_QUEUE_COMPUTE_BIT) && computeQueueNodeIndex != UINT32_MAX) {
		queueCreations[0] = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
							 .pNext = nullptr,
							 .flags = 0,
							 .queueFamilyIndex = this->graphics_queue_node_index,
							 .queueCount = nrQueues,
							 .pQueuePriorities = queue_priorities};
	}
	const VkDeviceQueueCreateInfo queueInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
											   .pNext = nullptr,
											   .flags = 0,
											   .queueFamilyIndex = this->graphics_queue_node_index,
											   .queueCount = nrQueues,
											   .pQueuePriorities = queue_priorities};

	/*  Required extensions.    */
	std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME, /*	*/
	};
	if (devices.size() > 1) {
	}

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
		deviceGroupDeviceCreateInfo.physicalDeviceCount = devices.size();
		// deviceGroupDeviceCreateInfo.pPhysicalDevices = devices.data();
	}

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = VK_NULL_HANDLE;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;

	// /*	Enable group if supported.	*/
	if (devices.size() > 1) {
		// deviceInfo.pNext = &deviceGroupDeviceCreateInfo;
	}
	deviceInfo.enabledExtensionCount = deviceExtensions.size();
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

	/*  Create device.  */
	vkCreateDevice(devices[0]->getHandle(), &deviceInfo, VK_NULL_HANDLE, &this->logicalDevice);

	/*  Get all queues.    */
	if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
		vkGetDeviceQueue(getHandle(), this->graphics_queue_node_index, 0, &this->graphicsQueue);
	if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
		vkGetDeviceQueue(getHandle(), this->present_queue_node_index, 0, &this->presentQueue);
	if (requiredQueues & VK_QUEUE_COMPUTE_BIT)
		vkGetDeviceQueue(getHandle(), this->compute_queue_node_index, 0, &this->computeQueue);

	this->physicalDevices = devices;
}

VKDevice::~VKDevice(void) {
	if (getHandle() != VK_NULL_HANDLE)
		vkDestroyDevice(getHandle(), VK_NULL_HANDLE);
}