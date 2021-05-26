#include "VKHelper.h"
#include <common.hpp>
#include <vulkan/vulkan.h>

std::optional<uint32_t> VKHelper::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
												 VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return {i};
		}
	}

	return {};
}

std::optional<uint32_t> VKHelper::findMemoryType(const VkPhysicalDeviceMemoryProperties &memProperties,
												 uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return {i};
		}
	}
	return {};
}

void VKHelper::createBuffer(VkDevice device, VkDeviceSize size, const VkPhysicalDeviceMemoryProperties &memoryProperies,
							VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
							VkDeviceMemory &bufferMemory) {
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
	const auto typeIndex = findMemoryType(memoryProperies, memRequirements.memoryTypeBits, properties);
	if (typeIndex)
		allocInfo.memoryTypeIndex = typeIndex.value();
	else
		throw std::runtime_error("");

	/**/
	if (vkAllocateMemory(device, &allocInfo, NULL, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	/**/
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VKHelper::createImage(VkDevice device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
						   VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
						   const VkPhysicalDeviceMemoryProperties &memProperties, VkImage &image,
						   VkDeviceMemory &imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memProperties, memRequirements.memoryTypeBits, properties).value();

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView VKHelper::createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
									  uint32_t mipLevels) {

	/**/
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

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

// TODO add option filter of what device you want.
void VKHelper::selectDefaultDevices(std::vector<VkPhysicalDevice> &devices,
									std::vector<VkPhysicalDevice> &selectDevices, uint32_t device_type_filter) {
	std::vector<VkPhysicalDevice> preliminaryDevices;
	/*  Check for matching. */
	// VK_KHR_device_group_creation
	// VK_KHR_device_group_creation
	/*	Check for the device with the best specs and can display.	*/

	for (int i = 0; i < devices.size(); i++) {
		const VkPhysicalDevice device = devices[i];
		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(device, &props);

		if ((props.deviceType & device_type_filter) == 0)
			continue;

		uint32_t nrDisplayProperties = 0;
		VK_CHECK(vkGetPhysicalDeviceDisplayPropertiesKHR(device, &nrDisplayProperties, nullptr));
		// if (nrDisplayProperties <= 0)
		// 	continue;

		std::vector<VkDisplayPropertiesKHR> displayProperties(nrDisplayProperties);
		VK_CHECK(vkGetPhysicalDeviceDisplayPropertiesKHR(device, &nrDisplayProperties, displayProperties.data()));

		/* Find all supported planes.	*/
		uint32_t planeCount;
		VK_CHECK(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(device, &planeCount, nullptr));
		// if (planeCount <= 0)
		// 	continue;

		for (int x = 0; x < displayProperties.size(); x++) {
		}

		// Determine the type of the physical device
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			preliminaryDevices.push_back(device);
		} else if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			/*	TODO determine if it can draw and display.	*/
			// preliminaryDevices.push_back(device);
		} else {
		}
	}

	for (int i = 0; i < preliminaryDevices.size(); i++) {
		const VkPhysicalDevice device = devices[i];
		// Determine the available device local memory.
		VkPhysicalDeviceMemoryProperties memoryProps = {};
		vkGetPhysicalDeviceMemoryProperties(device, &memoryProps);

		VkMemoryHeap *heapsPointer = memoryProps.memoryHeaps;
		std::vector<VkMemoryHeap> heaps =
			std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProps.memoryHeapCount);

		for (int j = 0; j < heaps.size(); j++) {
			VkMemoryHeap &heap = heaps[j];
			if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				selectDevices.push_back(device);
				// Device local heap, should be size of total GPU VRAM.
				// heap.size will be the size of VRAM in bytes. (bigger
				// is better)
			}
		}
	}
}

VkSurfaceFormatKHR VKHelper::selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats,
												 const std::vector<VkSurfaceFormatKHR> &requestFormats,
												 VkColorSpaceKHR request_color_space) {

	for (int request_i = 0; request_i < requestFormats.size(); request_i++) {
		for (uint32_t avail_i = 0; avail_i < availableFormats.size(); avail_i++) {
			if (availableFormats[avail_i].format == requestFormats[request_i].format &&
				availableFormats[avail_i].colorSpace == request_color_space)
				return availableFormats[avail_i];
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VKHelper::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes,
												 const std::vector<VkPresentModeKHR> &requestFormats, bool vsync) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto &availablePresentMode : availablePresentModes) {
		if (vsync && availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	for (const auto &availablePresentMode : availablePresentModes) {
		if (vsync && availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
			return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
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

void VKHelper::stageBufferCopy(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer src, VkBuffer dst,
							   VkDeviceSize size) {

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}