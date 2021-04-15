#include "VKDevice.h"

VKDevice::VKDevice(const std::vector<PhysicalDevice *> &devices,
				   std::unordered_map<const char *, bool> requested_extensions) {

	std::vector<VkPhysicalDevice> selectedDevices;
	// VKHelper::selectDefaultDevices(physicalDevices, selectedDevices);

	/*	*/
	// this->gpu = selectedDevices[0];

	// vkEnumerateDeviceLayerPropertie

	/*  Select queue with graphic.  */ // TODO add queue for compute and etc.
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t computeQueueNodeIndex;
	for (uint32_t i = 0; i < devices[0]->getQueueFamilyProperties().size(); i++) {
		const VkQueueFamilyProperties &prop = devices[0]->getQueueFamilyProperties()[i];
		if ((prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
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

	float queue_priorities[1] = {1.0};
	const VkDeviceQueueCreateInfo queue = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										   .pNext = NULL,
										   .flags = 0,
										   .queueFamilyIndex = this->graphics_queue_node_index,
										   .queueCount = 1,
										   .pQueuePriorities = queue_priorities};

	/*  Required extensions.    */
	std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME, /*	*/
										 // VK_NV_RAY_TRACING_EXTENSION_NAME
										 // VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
										 // VK_NV_GLSL_SHADER_EXTENSION_NAME
	};

	// // TODO resolve that it does if debug is enabled
	// if (this->enableValidationLayers) {
	// 	// TODO determine
	// 	deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	// }

	// VkDeviceGroupDeviceCreateInfo deviceGroupDeviceCreateInfo = {
	// 	.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
	// };
	// if (phyiscalGroupDevices.size() > 0) {
	// 	if (phyiscalGroupDevices[0].physicalDeviceCount > 1) {
	// 		deviceGroupDeviceCreateInfo.physicalDeviceCount = phyiscalGroupDevices[0].physicalDeviceCount;
	// 		deviceGroupDeviceCreateInfo.pPhysicalDevices = phyiscalGroupDevices[0].physicalDevices;
	// 	}
	// }

	VkPhysicalDeviceShaderDrawParameterFeatures deviceShaderDrawParametersFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES,
		.pNext = NULL,
		.shaderDrawParameters = VK_TRUE};
	VkPhysicalDeviceMultiviewFeatures deviceMultiviewFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
		.pNext = &deviceShaderDrawParametersFeatures,
		.multiview = VK_TRUE,
		.multiviewGeometryShader = VK_TRUE,
		.multiviewTessellationShader = VK_TRUE};
	VkPhysicalDeviceConditionalRenderingFeaturesEXT conditionalRenderingFeaturesExt = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT,
		.pNext = &deviceMultiviewFeatures,
		.conditionalRendering = VK_TRUE,
		.inheritedConditionalRendering = VK_TRUE};

	VkDeviceCreateInfo device = {};
	VkPhysicalDeviceFeatures deviceFeatures{};
	device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device.pNext = &conditionalRenderingFeaturesExt;
	device.queueCreateInfoCount = 1;
	device.pQueueCreateInfos = &queue;
	// if (this->enableValidationLayers) {
	// 	// device.enabledLayerCount = this->enabled_layer_count;
	// 	// device.ppEnabledLayerNames =
	// 	// 	(const char *const *)((this->enableValidationLayers) ? this->device_validation_layers : NULL);
	// } else {
	// 	device.enabledLayerCount = 0;
	// }

	// /*	Enable group if supported.	*/
	// if (deviceGroupDeviceCreateInfo.physicalDeviceCount > 1) {
	// 	deviceShaderDrawParametersFeatures.pNext = &deviceGroupDeviceCreateInfo;
	// }
	device.enabledExtensionCount = deviceExtensions.size();
	device.ppEnabledExtensionNames = deviceExtensions.data();
	device.pEnabledFeatures = &deviceFeatures;

	/*  Create device.  */
	vkCreateDevice(devices[0]->getHandle(), &device, NULL, &this->logicalDevice);

	/*  Get all queues.    */
	vkGetDeviceQueue(getHandle(), this->graphics_queue_node_index, 0, &this->graphicsQueue);
	vkGetDeviceQueue(getHandle(), this->graphics_queue_node_index, 0, &this->presentQueue);
	vkGetDeviceQueue(getHandle(), this->compute_queue_node_index, 0, &this->computeQueue);

	this->mDevices = devices;
}
// VKDevice::VKDevice(const std::vector<VkPhysicalDevice> &devices,
// 				   std::unordered_map<const char *, bool> requested_extensions) {

// }