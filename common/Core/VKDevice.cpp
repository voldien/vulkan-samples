#include "VKDevice.h"

VKDevice::VKDevice(const std::vector<std::shared_ptr<PhysicalDevice>> &devices,
				   const std::unordered_map<const char *, bool> &requested_extensions, VkQueueFlags requiredQueues) {

	/*  Select queue with graphic.  */
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t computeQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	uint32_t transferQueueNodeIndex = UINT32_MAX;

	/*  Required extensions.    */
	std::vector<const char *> deviceExtensions;
	deviceExtensions.reserve(requested_extensions.size());

	for (uint32_t j = 0; j < devices.size(); j++) {
		const std::shared_ptr<PhysicalDevice> &device = devices[j];
		/*	Iterate through each extension and add if supported.	*/
		for (const std::pair<const char *, bool> &n : requested_extensions) {
			if (n.second) {
				if (device->isExtensionSupported(n.first))
					deviceExtensions.push_back(n.first);
				else
					throw std::runtime_error(
						fmt::format("{} does not support: {}\n", device->getDeviceName(), n.first));
			}
		}
	}

	uint32_t nrQueues = 1;
	for (uint32_t j = 0; j < devices.size(); j++) {
		const std::shared_ptr<PhysicalDevice> &phDevice = devices[j];

		for (uint32_t i = 0; i < phDevice->getQueueFamilyProperties().size(); i++) {
			/*  */
			const VkQueueFamilyProperties &familyProp = phDevice->getQueueFamilyProperties()[i];

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
	}

	// TODO resolve in case compute is not supported on the graphic queue.
	if ((requiredQueues & VK_QUEUE_COMPUTE_BIT) && computeQueueNodeIndex == UINT32_MAX) {
	}
	// TODO check so tha all the required required queues were found.

	this->graphics_queue_node_index = graphicsQueueNodeIndex;
	this->compute_queue_node_index = computeQueueNodeIndex;
	this->present_queue_node_index = graphics_queue_node_index;
	this->transfer_queue_node_index = graphics_queue_node_index;

	std::vector<VkDeviceQueueCreateInfo> queueCreations(nrQueues);
	std::vector<float> queuePriorities(1.0f, nrQueues);
	//if ((requiredQueues & VK_QUEUE_COMPUTE_BIT) && computeQueueNodeIndex != UINT32_MAX) {
		VkDeviceQueueCreateInfo &queueCreateInfo = queueCreations[0];
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = this->graphics_queue_node_index;
		queueCreateInfo.queueCount = nrQueues;
		queueCreateInfo.pQueuePriorities = queuePriorities.data();
	//}
	// std::vector<VkDeviceQueueCreateInfo> queueInfos(1);

	// queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	// queueInfos[0].pNext = nullptr;
	// queueInfos[0].flags = 0;
	// queueInfos[0].queueFamilyIndex = this->graphics_queue_node_index;
	// queueInfos[0].queueCount = nrQueues;
	// queueInfos[0].pQueuePriorities = queuePriorities.data();

	/*	*/
	VkDeviceGroupDeviceCreateInfo deviceGroupDeviceCreateInfo{};
	deviceGroupDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;

	/*	*/
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = VK_NULL_HANDLE;
	deviceInfo.queueCreateInfoCount = queueCreations.size();
	deviceInfo.pQueueCreateInfos = queueCreations.data();

	// /*	Enable group if supported.	*/
	std::vector<VkPhysicalDevice> groupDevices(devices.size());
	if (devices.size() > 1) {
		for (size_t i = 0; i < devices.size(); i++)
			groupDevices[i] = devices[i]->getHandle();
		deviceGroupDeviceCreateInfo.physicalDeviceCount = groupDevices.size();
		deviceGroupDeviceCreateInfo.pPhysicalDevices = groupDevices.data();
		deviceInfo.pNext = &deviceGroupDeviceCreateInfo;
	}

	deviceInfo.enabledExtensionCount = deviceExtensions.size();
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

	/*  Create device.  */
	VKS_VALIDATE(vkCreateDevice(devices[0]->getHandle(), &deviceInfo, VK_NULL_HANDLE, &this->logicalDevice));

	/*  Get all queues.    */
	if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
		vkGetDeviceQueue(getHandle(), this->graphics_queue_node_index, 0, &this->graphicsQueue);
	if (requiredQueues & VK_QUEUE_GRAPHICS_BIT)
		vkGetDeviceQueue(getHandle(), this->present_queue_node_index, 0, &this->presentQueue);
	if (requiredQueues & VK_QUEUE_COMPUTE_BIT)
		vkGetDeviceQueue(getHandle(), this->compute_queue_node_index, 0, &this->computeQueue);
	if (requiredQueues & VK_QUEUE_TRANSFER_BIT)
		vkGetDeviceQueue(getHandle(), this->transfer_queue_node_index, 0, &this->transferQueue);

	this->physicalDevices = devices;
}

VKDevice::VKDevice(const std::shared_ptr<PhysicalDevice> &physicalDevice,
				   const std::unordered_map<const char *, bool> &requested_extensions, VkQueueFlags requiredQueues) {
	const std::vector<std::shared_ptr<PhysicalDevice>> physical = {physicalDevice};
	VKDevice(physical, requested_extensions, requiredQueues);
}

VKDevice::~VKDevice(void) {
	if (getHandle() != VK_NULL_HANDLE)
		vkDestroyDevice(getHandle(), VK_NULL_HANDLE);
}