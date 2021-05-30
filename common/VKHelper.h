#ifndef _VKSAMPLES_VK_HELPER_H_
#define _VKSAMPLES_VK_HELPER_H_ 1
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define ArraySize(a) (sizeof(a) / sizeof(*a))

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
	static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
									   uint32_t mipLevels);

//	template<typename T>
	static void createSampler(VkDevice device, VkSampler &sampler, float maxSamplerAnisotropy = 1.0f, void* pNext = nullptr) {

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
		samplerInfo.anisotropyEnable = VK_TRUE;
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
	static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &data){
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = data.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(data.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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
									 const std::vector<VkPushConstantRange> &pushConstants = {}, void *next = NULL);

	/**
	 * @brief Create a Descriptor Set Layout object
	 *
	 * @param device
	 * @param descriptorSetLayout
	 * @param descitprSetLayoutBindings
	 * @param pNext
	 */
	static void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout &descriptorSetLayout,
										  const std::vector<VkDescriptorSetLayoutBinding> &descitprSetLayoutBindings,
										  void *pNext = nullptr) {
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.pNext = pNext;
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = descitprSetLayoutBindings.size();
		layoutInfo.pBindings = descitprSetLayoutBindings.data();

		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
	}

	static VkPipeline createGraphicPipeline(void);

	static VkPipeline createComputePipeline(void);

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
		uint32_t graphicsFamily = -1;
		uint32_t presentFamily = -1;

		bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
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
								VkDeviceSize size);

	static void stageBufferCmdCopy(VkDevice device, VkQueue queue, VkCommandBuffer cmd, VkBuffer src, VkBuffer dst,
								   VkDeviceSize size);
};

#define CHECK_VK_ERROR(result)

#endif
