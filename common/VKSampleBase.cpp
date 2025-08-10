#include "VKSampleBase.h"
#include "Core/Library.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "vulkan/vulkan_core.h"

#include <renderdoc_app.h>

using namespace vksample;

VKSampleSessionBase::VKSampleSessionBase(std::shared_ptr<fvkcore::VulkanCore> &core,
										 std::shared_ptr<fvkcore::VKDevice> &device)
	: core(core), device(device) {

	this->getTimer().start();

	/* Create logger	*/
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_level(spdlog::level::trace);
	stdout_sink->set_color_mode(spdlog::color_mode::always);
	stdout_sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
	stdout_sink->set_pattern("%g:%# [%^%l%$] %v");

	/*	*/
	this->logger = new spdlog::logger("vulkan-sample", {stdout_sink});
	this->logger->set_level(spdlog::level::trace);

	/*	Verify Features and Properties.	*/
	{
		VkPhysicalDevicePerStageDescriptorSetFeaturesNV perStageDescFeature;
		this->getPhysicalDevice()->checkFeature(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PER_STAGE_DESCRIPTOR_SET_FEATURES_NV,
												perStageDescFeature);

		VkPhysicalDeviceMultiviewFeatures multiviewFeatures;

		VkPhysicalDeviceHostImageCopyFeatures hostImageCopyFeatures;
		this->getPhysicalDevice()->checkFeature(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT,
												hostImageCopyFeatures);
		this->useHostImageCopy = false; // hostImageCopyFeatures.hostImageCopy;

		VkFormat image_format = VK_FORMAT_R8_SNORM;

		VkFormatProperties3 format_properties_3{};
		format_properties_3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR;

		// Properties3 need to be chained into Properties2
		VkFormatProperties2 format_properties_2{};
		format_properties_2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
		format_properties_2.pNext = &format_properties_3;

		// Get format properties for the select image format
		vkGetPhysicalDeviceFormatProperties2(this->getPhysicalDevice()->getHandle(), image_format,
											 &format_properties_2);
		if ((format_properties_3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT) == 0) {
			// Fallback to a different format or use other means of uploading data
		}

		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeaturesEXT;
		VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2FeaturesEXT;
		VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT;
		this->getPhysicalDevice()->checkFeature(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
												extendedDynamicStateFeaturesEXT);
		this->getPhysicalDevice()->checkFeature(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT,
												extendedDynamicState2FeaturesEXT);
		this->getPhysicalDevice()->checkFeature(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
												extendedDynamicState3FeaturesEXT);

		hasDynamicState =
			this->getPhysicalDevice()->isExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) &&
			extendedDynamicStateFeaturesEXT.extendedDynamicState;
		hasDynamicState2 =
			this->getPhysicalDevice()->isExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME) &&
			extendedDynamicState2FeaturesEXT.extendedDynamicState2;
		hasDynamicState3 =
			this->getPhysicalDevice()->isExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) &&
			extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEnable &&
			extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEquation;
	}

	/*	*/
	this->loadDefaultQueue();

	/*	*/
	this->loadDescriptorPool();

	VkQueryPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;

	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	createInfo.queryCount = 2; // TODO

	/*	*/
	VKS_VALIDATE(vkCreateQueryPool(this->getDevice(), &createInfo, this->getAllocatorCallback(), &queryPool));
}

VKSampleSessionBase::~VKSampleSessionBase() {

	this->release();

	/*	*/
	if (this->graphic_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(getDevice(), this->graphic_pool, this->getAllocatorCallback());
		this->graphic_pool = VK_NULL_HANDLE;
	}
	if (this->transfer_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(getDevice(), this->transfer_pool, this->getAllocatorCallback());
		this->transfer_pool = VK_NULL_HANDLE;
	}
	if (this->compute_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(getDevice(), this->compute_pool, this->getAllocatorCallback());
		this->compute_pool = VK_NULL_HANDLE;
	}

	/*	*/
	vkDestroyDescriptorPool(this->getDevice(), this->getDescriptorPool(), this->getAllocatorCallback());

	vkDestroyPipelineCache(this->getDevice(), this->getPipelineCache(), this->getAllocatorCallback());

	vkDestroyQueryPool(this->getDevice(), queryPool, this->getAllocatorCallback());
}

void VKSampleSessionBase::loadDefaultQueue() {

	/*	*/
	const std::vector<fvkcore::VKDevice::VKQueue> &queues = this->device->getQueues();
	const std::vector<VkQueueFamilyProperties> &physical_queus = this->getPhysicalDevice()->getQueueFamilyProperties();

	/*	*/
	for (size_t i = 0; i < queues.size(); i++) {
		if (physical_queus[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
		}
		// queues[i].
	}

	/*	*/
	this->graphics_queue_node_index = 0;
	this->compute_queue_node_index = 0;
	this->transfer_queue_node_index = 0;

	/*	*/
	this->graphic_queue = this->device->getQueue(this->graphics_queue_node_index, 0);
	this->compute_queue = this->device->getQueue(this->compute_queue_node_index, 0);
	this->transfer_queue = this->device->getQueue(this->transfer_queue_node_index, 0);
}

void VKSampleSessionBase::loadDescriptorPool() {

	/*	Create Descriptor Pool.	*/
	std::vector<VkDescriptorPoolSize> poolSize = {{VK_DESCRIPTOR_TYPE_SAMPLER, 128},
												  {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
												  {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128},
												  {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 128},
												  {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 128},
												  {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 128},
												  {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128},
												  {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128},
												  {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 128},
												  {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 128},
												  {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 128},
												  {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 128}};

	const size_t max_descriptor_sets = 8192;

	VkDescriptorPoolCreateFlags flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	this->desc_pool = fvkcore::VKHelper::createDescPool(this->getDevice(), poolSize, flags, max_descriptor_sets,
														this->getAllocatorCallback(), nullptr);
}

void VKSampleSessionBase::loadCachePipeline() {
	const VkPipelineCacheCreateFlags cacheFlags = 0;
	this->pipelineCache = fvkcore::VKHelper::createPipelineCache(this->getDevice(), 0, nullptr, cacheFlags,
																 this->getAllocatorCallback(), nullptr);
}

void VKSampleSessionBase::allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags,
										 VkBuffer &buffer, VkDeviceMemory &bufferMemory, void *pNext) {

	fvkcore::VKHelper::createBuffer(getVKDevice()->getHandle(), size,
									getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(), usage, flags, buffer,
									bufferMemory);
}

void VKSampleSessionBase::allocateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
										VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
										const VkImageCreateFlags flags, VkImage &image, VkDeviceMemory &imageMemory,
										void *pNext) {
	/*	*/
	fvkcore::VKHelper::createImage2D(this->getVKDevice()->getHandle(), width, height, mipLevels, format, tiling, usage,
									 properties, getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(), flags,
									 image, imageMemory);
}

static VkDeviceMemory stageBufferMemory = VK_NULL_HANDLE;
static VkBuffer stageBuffer = VK_NULL_HANDLE;

void VKSampleSessionBase::transferBufferData(VkBuffer buffer, VkDeviceMemory memory, const void *pData,
											 const size_t sizeInBytes, const size_t offset) {

	VkDevice device = this->getDevice();

	if (!this->getPhysicalDevice()->isLocalandStaging()) {
		// stageBufferMemory
		if (stageBufferMemory != VK_NULL_HANDLE) {
			vkDestroyBuffer(getDevice(), stageBuffer, nullptr);
			vkFreeMemory(getDevice(), stageBufferMemory, nullptr);
			stageBufferMemory = VK_NULL_HANDLE;
		}

		if (stageBufferMemory == VK_NULL_HANDLE) {
			this->allocateBuffer(sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
								 stageBuffer, stageBufferMemory);
		}

		void *mapBufer = nullptr;
		vkMapMemory(device, stageBufferMemory, 0, sizeInBytes, 0, &mapBufer);
		memcpy(mapBufer, pData, sizeInBytes);
		vkUnmapMemory(device, stageBufferMemory);

		fvkcore::VKHelper::stageBufferCopy(device, this->transfer_queue, transfer_pool, stageBuffer, buffer,
										   sizeInBytes);

	} else {

		void *mapBufer = nullptr;
		vkMapMemory(device, memory, 0, sizeInBytes, 0, &mapBufer);
		memcpy(mapBufer, pData, sizeInBytes);
		vkUnmapMemory(device, memory);
	}
}

void VKSampleSessionBase::transferImageData(VkImage image, VkDeviceMemory imageMemory, const size_t width,
											const size_t height, const size_t depth, const void *pData,
											const size_t sizeInBytes, const size_t offset) {

	VkDevice device = this->getDevice();

	VkCommandBuffer cmd = getTransferCommandBuffer();

	VkHostImageLayoutTransitionInfoEXT host_image_layout_transition_info{};
	host_image_layout_transition_info.sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT;
	host_image_layout_transition_info.image = image;
	host_image_layout_transition_info.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	host_image_layout_transition_info.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// host_image_layout_transition_info.subresourceRange = subresource_range;

	if (this->useHostImageCopy) {

		// Setup host to image copy
		VkMemoryToImageCopyEXT memory_to_image_copy{};

		memory_to_image_copy.sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT;
		memory_to_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memory_to_image_copy.imageSubresource.mipLevel = 0;
		memory_to_image_copy.imageSubresource.baseArrayLayer = 0;
		memory_to_image_copy.imageSubresource.layerCount = 1;
		memory_to_image_copy.imageExtent.width = width;
		memory_to_image_copy.imageExtent.height = height;
		memory_to_image_copy.imageExtent.depth = depth;
		memory_to_image_copy.pHostPointer = pData;

		// Issue the copy
		VkCopyMemoryToImageInfoEXT copy_memory_info{};
		copy_memory_info.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT;
		copy_memory_info.dstImage = image;
		copy_memory_info.dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		copy_memory_info.regionCount = 1; // TODO: fix  - : static_cast<uint32_t>(memory_to_image_copies.size());
		copy_memory_info.pRegions = &memory_to_image_copy;

		// vkCopyMemoryToImageEXT(device, &copy_memory_info);

	} else {

		/*	Wait intill ready to transfer image.	*/
		fvkcore::VKHelper::transitionImageLayout(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
												 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkDeviceMemory stageMemory = nullptr;
		VkBuffer stageBuffer = nullptr;
		this->allocateBuffer(sizeInBytes, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
							 stageBuffer, stageMemory);

		void *mapBufer = nullptr;

		vkMapMemory(device, stageMemory, 0, sizeInBytes, 0, &mapBufer);
		memcpy(mapBufer, pData, sizeInBytes);
		vkUnmapMemory(device, stageMemory);

		if (this->getPhysicalDevice()->isLocalandStaging()) {

			fvkcore::VKHelper::copyBufferToImageCmd(
				cmd, stageBuffer, image,
				{static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(depth)});

		} else {

			fvkcore::VKHelper::copyBufferToImageCmd(
				cmd, stageBuffer, image,
				{static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(depth)});

			//	VKHelper::transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_UNDEFINED,
		}
	}

	endTransferCommand(cmd);
}

VkCommandBuffer VKSampleSessionBase::getTransferCommandBuffer() const noexcept {
	VkCommandPool transferPool = this->transfer_pool;
	VkCommandBuffer cmd = this->getVKDevice()->beginSingleTimeCommands(transferPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
																	   VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)[0];

	return cmd;
}
void VKSampleSessionBase::endTransferCommand(VkCommandBuffer cmd) const noexcept {
	VkQueue transferQueue = this->transfer_queue;
	VkCommandPool transferPool = this->transfer_pool;

	this->getVKDevice()->endSingleTimeCommands(transferQueue, cmd, transferPool);
}

void VKSampleSessionBase::debug(const bool enable) {

	if (enable) {
		this->logger->set_level(spdlog::level::trace);
	} else {
		this->logger->set_level(spdlog::level::info);
	}

	try {
		/*	*/
		fragcore::Library library("librenderdoc.so");

		/*	*/
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)library.getfunc("RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (&this->rdoc_api));

		assert(ret == 1);

	} catch (const std::exception &ex) {
		this->logger->warn(ex.what());
	}
}