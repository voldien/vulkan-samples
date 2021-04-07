#include "VKHelper.h"
#include <vulkan/vulkan.h>

uint32_t VKHelper::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
								  VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

uint32_t VKHelper::findMemoryType(VkPhysicalDeviceMemoryProperties *memProperties, uint32_t typeFilter,
								  VkMemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < memProperties->memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
			(memProperties->memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
}

void VKHelper::createBuffer(VkDevice device, VkDeviceSize size,
							VkPhysicalDeviceMemoryProperties* memoryProperies, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
							VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
	VkResult result;

	/**/
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	/**/
	if (vkCreateBuffer(device, &bufferInfo, NULL, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	/**/
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memoryProperies, memRequirements.memoryTypeBits, properties);

	/**/
	if (vkAllocateMemory(device, &allocInfo, NULL, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	/**/
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

// VkImageView VKHelper::createImageView(VulkanCore *vulkanCore, VkImage image, VkFormat format) {
// 	/**/
// 	VkImageViewCreateInfo viewInfo = {};
// 	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
// 	viewInfo.image = image;
// 	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
// 	viewInfo.format = format;
// 	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
// 	viewInfo.subresourceRange.baseMipLevel = 0;
// 	viewInfo.subresourceRange.levelCount = 1;
// 	viewInfo.subresourceRange.baseArrayLayer = 0;
// 	viewInfo.subresourceRange.layerCount = 1;

// 	VkImageView imageView;
// 	if (vkCreateImageView(vulkanCore->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
// 		throw std::runtime_error("");
// 	}

// 	return imageView;
// }

VkShaderModule VKHelper::createShaderModule(VkDevice device, std::vector<char> &data) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = data.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(data.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

// bool VKHelper::isDeviceSuitable(VkPhysicalDevice device) {
// 	VkPhysicalDeviceProperties deviceProperties;
// 	VkPhysicalDeviceFeatures deviceFeatures;
// 	VkQueueFamilyProperties a;

// 	/*  */
// 	vkGetPhysicalDeviceProperties(device, &deviceProperties);
// 	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

// 	bool swapChainAdequate = false;
// 	//	vkGetPhysicalDeviceMemoryProperties()
// 	//	vkGetPhysicalDeviceQueueFamilyProperties()
// 	//  vkGetPhysicalDeviceQueueFamilyProperties(vulkancore->gpu,
// 	//  &vulkancore->queue_count, NULL);
// 	/*  Check if device is good enough as a GPU candidates.  */

// 	// TODO determine, since it adds the requirement of a display in order to use the device.
// 	uint32_t displayCount;
// 	std::vector<VkDisplayPropertiesKHR> displayProperties;
// 	vkGetPhysicalDeviceDisplayPropertiesKHR(device, &displayCount, NULL);
// 	/*  */

// 	return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
// 			deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
// 			deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) &&
// 		   deviceFeatures.geometryShader && deviceFeatures.samplerAnisotropy && displayCount > 0;
// }

void VKHelper::selectDefaultDevices(std::vector<VkPhysicalDevice> &devices,
									std::vector<VkPhysicalDevice> &selectDevices) {
	/*  Check for matching. */
	// VK_KHR_device_group_creation
	// VK_KHR_device_group_creation
	/*	Check for the device with the best specs and can display.	*/

	for (int i = 0; i < devices.size(); i++) {
		const VkPhysicalDevice &device = devices[i];
		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(device, &props);

		uint32_t count;
		std::vector<VkDisplayPropertiesKHR> displayProperties;
		vkGetPhysicalDeviceDisplayPropertiesKHR(device, &count, NULL);
		displayProperties.resize(count);
		vkGetPhysicalDeviceDisplayPropertiesKHR(device, &count, displayProperties.data());

		// Determine the type of the physical device
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			selectDevices.push_back(device);
		} else if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			/*	TODO determine if it can draw and display.	*/

			// selectDevices.push_back(device);
			// You've got yourself an integrated GPU.
		} else {
			// I don't even...
		}

		// Determine the available device local memory.
		VkPhysicalDeviceMemoryProperties memoryProps = {};
		vkGetPhysicalDeviceMemoryProperties(device, &memoryProps);

		VkMemoryHeap *heapsPointer = memoryProps.memoryHeaps;
		std::vector<VkMemoryHeap> heaps =
			std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProps.memoryHeapCount);

		for (int j = 0; j < heaps.size(); j++) {
			VkMemoryHeap &heap = heaps[j];
			if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				// Device local heap, should be size of total GPU VRAM.
				// heap.size will be the size of VRAM in bytes. (bigger
				// is better)
			}
		}
	}
}

VkSurfaceFormatKHR VKHelper::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	// if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
	// 	return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	// }

	for (const auto &availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR VKHelper::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto &availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			//return availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VKHelper::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D actualExtent) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {

		actualExtent.width = std::max(capabilities.minImageExtent.width,
									  std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
									   std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

VKHelper::QueueFamilyIndices VKHelper::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

VKHelper::SwapChainSupportDetails VKHelper::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}
