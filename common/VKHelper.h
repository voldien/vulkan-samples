#ifndef _VKSAMPLES_VK_HELPER_H_
#define _VKSAMPLES_VK_HELPER_H_ 1
#include <array>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define ArraySize(a) (sizeof(a) / sizeof(*a))

/**
 * 
 */
class VKHelper {
  public:
	/*  Helper functions.   */

	/**
	 * @brief
	 *
	 * @param physicalDevice
	 * @param typeFilter
	 * @param properties
	 * @return uint32_t
	 */
	static std::optional<uint32_t> findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
												  VkMemoryPropertyFlags properties);

	/**
	 * @brief
	 *
	 * @param memProperties
	 * @param typeFilter
	 * @param properties
	 * @return uint32_t
	 */
	static std::optional<uint32_t> findMemoryType(const VkPhysicalDeviceMemoryProperties &memProperties,
												  uint32_t typeFilter, VkMemoryPropertyFlags properties);

	/**
	 * @brief
	 *
	 * @param commandBuffer
	 * @param image
	 * @param format
	 * @param oldLayout
	 * @param newLayout
	 */
	static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout,
									  VkImageLayout newLayout) {

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		/*	*/
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	static void createMemory(VkDevice device, VkDeviceSize size, VkMemoryPropertyFlags properties,
							 const VkPhysicalDeviceMemoryProperties &memoryProperies, VkDeviceMemory &deviceMemory) {
		/**/
		// VkMemoryAllocateInfo allocInfo = {};
		// allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		// allocInfo.allocationSize = size;
		// const auto typeIndex = findMemoryType(memoryProperies, memRequirements.memoryTypeBits, properties);
		// if (typeIndex)
		// 	allocInfo.memoryTypeIndex = typeIndex.value();
		// else
		// 	throw std::runtime_error("");

		// /**/
		// VkResult result = vkAllocateMemory(device, &allocInfo, NULL, &deviceMemory);
//			throw std::runtime_error("failed to allocate buffer memory!");
//		}
	}

	/**
	 * @brief Create a Buffer object
	 *
	 * @param device
	 * @param size
	 * @param memoryProperies
	 * @param usage
	 * @param properties
	 * @param buffer
	 * @param bufferMemory
	 */
	static void createBuffer(VkDevice device, VkDeviceSize size,
							 const VkPhysicalDeviceMemoryProperties &memoryProperies, VkBufferUsageFlags usage,
							 VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

	static void createImage(VkDevice device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
							VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
							const VkPhysicalDeviceMemoryProperties &memProperties, VkImage &image,
							VkDeviceMemory &imageMemory);

	/**
	 * @brief Create a Image View object
	 *
	 * @param device
	 * @param image
	 * @param format
	 * @return VkImageView
	 */
	static VkImageView createImageView(VkDevice device, VkImage image, VkImageViewType imageType, VkFormat format,
									   VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	//	template<typename T>
	static void createSampler(VkDevice device, VkSampler &sampler, float maxSamplerAnisotropy = 1.0f,
							  void *pNext = nullptr) {

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.pNext = pNext;
		samplerInfo.flags = 0;
		/*	*/
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		/**/
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		/*	*/
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		/*	*/
		samplerInfo.mipLodBias = 0;
		/*	*/
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = maxSamplerAnisotropy;
		/*	*/
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		/**/
		samplerInfo.maxLod = 0;
		samplerInfo.minLod = 0;
		/**/
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	/**
	 * @brief Create a Shader Module object
	 *
	 * @param device
	 * @param data
	 * @return VkShaderModule
	 */
	static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &data,
											 const VkAllocationCallbacks *pAllocator = nullptr,
											 const char *pNext = nullptr) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = pNext;
		createInfo.flags = 0;
		createInfo.codeSize = data.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(data.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, pAllocator, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	/**
	 * @brief Create a Pipeline Layout object
	 *
	 * @param device
	 * @param pipelineLayout
	 * @param descLayouts
	 * @param pushConstants
	 * @param next
	 */
	static void createPipelineLayout(VkDevice device, VkPipelineLayout &pipelineLayout,
									 const std::vector<VkDescriptorSetLayout> &descLayouts = {},
									 const std::vector<VkPushConstantRange> &pushConstants = {}, void *pNext = nullptr);

	/**
	 * @brief Create a Descriptor Set Layout object
	 *
	 * @tparam n
	 * @param device
	 * @param descriptorSetLayout
	 * @param descitprSetLayoutBindings
	 * @param pAllocator
	 * @param pNext
	 */
	template <size_t n>
	static void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout &descriptorSetLayout,
										  const std::array<VkDescriptorSetLayoutBinding, n> &descitprSetLayoutBindings,
										  const VkAllocationCallbacks *pAllocator = nullptr, void *pNext = nullptr) {

		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingsV(descitprSetLayoutBindings.begin(),
																			   descitprSetLayoutBindings.end());

		createDescriptorSetLayout(device, descriptorSetLayout, descriptorSetLayoutBindingsV, pAllocator, pNext);
	}

	/**
	 * @brief Create a Descriptor Set Layout object
	 *
	 * @param device
	 * @param descriptorSetLayout
	 * @param descitprSetLayoutBindings
	 * @param pAllocator
	 * @param pNext
	 */
	static void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout &descriptorSetLayout,
										  const std::vector<VkDescriptorSetLayoutBinding> &descitprSetLayoutBindings,
										  const VkAllocationCallbacks *pAllocator = nullptr, void *pNext = nullptr) {
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.pNext = pNext;
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = descitprSetLayoutBindings.size();
		layoutInfo.pBindings = descitprSetLayoutBindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, pAllocator, &descriptorSetLayout);
	}

	static VkDescriptorPool createDescPool(VkDevice device, const std::vector<VkDescriptorPoolSize> &poolSizes = {},
										   uint32_t maxSets = 1, const VkAllocationCallbacks *pAllocator = nullptr,
										   void *pNext = nullptr) {
		VkDescriptorPool descPool;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = pNext;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		VkResult result = vkCreateDescriptorPool(device, &poolInfo, pAllocator, &descPool);

		return descPool;
	}

	static VkPipelineCache createPipelineCache(VkDevice device, int size, void *pdata,
											   const VkAllocationCallbacks *pAllocator = nullptr,
											   void *pNext = nullptr) {

		VkPipelineCache pipelineCache;

		VkPipelineCacheCreateInfo pipelineCacheInfo{};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipelineCacheInfo.pNext = pNext;
		pipelineCacheInfo.pInitialData = pdata;
		pipelineCacheInfo.initialDataSize = size;
		pipelineCacheInfo.flags = 0;

		VkResult result = vkCreatePipelineCache(device, &pipelineCacheInfo, pAllocator, &pipelineCache);

		return pipelineCache;
	}

	static VkPipeline createGraphicPipeline(void);

	static VkPipeline createComputePipeline(VkDevice device, VkPipelineLayout layout,
											const VkPipelineShaderStageCreateInfo &compShaderStageInfo,
											VkPipelineCache pipelineCache = VK_NULL_HANDLE,
											VkPipeline basePipelineHandle = VK_NULL_HANDLE,
											uint32_t basePipelineIndex = 0,
											const VkAllocationCallbacks *pAllocator = nullptr, void *pNext = nullptr) {

		VkPipeline pipeline;
		VkComputePipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.pNext = pNext;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stage = compShaderStageInfo;
		pipelineCreateInfo.layout = layout;
		pipelineCreateInfo.basePipelineHandle = basePipelineHandle;
		pipelineCreateInfo.basePipelineIndex = basePipelineIndex;

		VkResult result =
			vkCreateComputePipelines(device, pipelineCache, 1, &pipelineCreateInfo, pAllocator, &pipeline);

		return pipeline;
	}

	//
	// static bool isDeviceSuitable(VkPhysicalDevice device);

	/**
	 * @brief
	 *
	 * @param devices
	 * @param selectDevices
	 * @param device_type_filter
	 */
	static void selectDefaultDevices(std::vector<VkPhysicalDevice> &devices,
									 std::vector<VkPhysicalDevice> &selectDevices,
									 uint32_t device_type_filter = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU |
																   VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

	// TODO improve to accomudate the configurations.
	/**
	 * @brief
	 *
	 * @param availableFormats
	 * @return VkSurfaceFormatKHR
	 */
	static VkSurfaceFormatKHR selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats,
												  const std::vector<VkSurfaceFormatKHR> &requestFormats,
												  VkColorSpaceKHR request_color_space);

	/**
	 * @brief
	 *
	 * @param availablePresentModes
	 * @param vsync
	 * @return VkPresentModeKHR
	 */
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes,
												  const std::vector<VkPresentModeKHR> &requestFormats, bool vsync);

	/**
	 * @brief
	 *
	 * @param capabilities
	 * @param actualExtent
	 * @return VkExtent2D
	 */
	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D actualExtent);

	struct QueueFamilyIndices {
		int32_t graphicsFamily = -1;
		int32_t presentFamily = -1;

		bool isComplete() noexcept { return graphicsFamily != -1 && presentFamily != -1; }
	};

	/**
	 * @brief
	 *
	 * @param device
	 * @param surface
	 * @return QueueFamilyIndices
	 */
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	/**
	 * @brief
	 *
	 * @param device
	 * @param queue
	 * @param commandPool
	 * @param src
	 * @param dst
	 * @param size
	 */
	static void stageBufferCopy(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer src, VkBuffer dst,
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

	static void stageBufferCmdCopy(VkDevice device, VkQueue queue, VkCommandBuffer cmd, VkBuffer src, VkBuffer dst,
								   VkDeviceSize size) {}
	static void stageBufferToImageCmdCopyDirect(VkDevice device, VkQueue queue, VkCommandBuffer cmd, VkBuffer src,
												VkImage dst, const VkExtent3D &size,
												const VkOffset3D &offset = {0, 0, 0}) {

		// VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		// VkBufferImageCopy region{};
		// region.bufferOffset = 0;
		// region.bufferRowLength = 0;
		// region.bufferImageHeight = 0;
		// region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// region.imageSubresource.mipLevel = 0;
		// region.imageSubresource.baseArrayLayer = 0;
		// region.imageSubresource.layerCount = 1;
		// region.imageOffset = {0, 0, 0};
		// region.imageExtent = {width, height, 1};

		// vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// endSingleTimeCommands(commandBuffer);
	}

	static void copyBufferToImageCmd(VkCommandBuffer cmd, VkBuffer src, VkImage dst, const VkExtent3D &size,
									 const VkOffset3D &offset = {0, 0, 0}) {

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = offset;
		region.imageExtent = size;

		vkCmdCopyBufferToImage(cmd, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
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

		return commandBuffer;
	}

	static void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer,
									  VkCommandPool commandPool) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}
};

#define CHECK_VK_ERROR(result)

#endif
