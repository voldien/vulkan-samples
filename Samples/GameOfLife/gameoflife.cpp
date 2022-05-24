#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <VKWindow.h>

class GameOfLife : public VKWindow {
  private:
	VkPipeline computePipeline = VK_NULL_HANDLE;
	VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

	/*	*/
	std::vector<VkImage> gameOfLifeImage;
	std::vector<VkDeviceMemory> mandelBrotImageMemory;

	/*	*/
	std::vector<VkImageView> computeImageViews;
	std::vector<VkBuffer> cellsBuffers;
	std::vector<VkDeviceMemory> cellsMemory;

	/*	*/
	std::vector<VkDeviceMemory> paramMemory;
	VkBuffer paramBuffer = VK_NULL_HANDLE;

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;

	std::vector<VkDescriptorSet> descriptorSets;

	struct reaction_diffusion_param_t {
		float kernelA[4][4];
		float kernelB[4][4];
		float feedRate = 0.1f;
		float killRate = .3;
		float diffuseRateA = .1;
		float diffuseRateB = .3;
		float delta = .01;

		/**/
		float posX, posY;
		float mousePosX, mousePosY;
		float zoom; /*  */
		float c;	/*  */
	} params = {};

	size_t paramMemSize = sizeof(params);

  public:
	GameOfLife(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->setTitle(std::string("Game of Life - Compute"));
	}
	virtual ~GameOfLife() {}

	virtual void release() override {
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);
		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);

		for (unsigned int i = 0; i < computeImageViews.size(); i++)
			vkDestroyImageView(getDevice(), computeImageViews[i], nullptr);

		for (unsigned int i = 0; i < paramMemory.size(); i++)
			vkFreeMemory(getDevice(), paramMemory[i], nullptr);

		vkDestroyBuffer(getDevice(), paramBuffer, nullptr);
		vkDestroyPipeline(getDevice(), computePipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), computePipelineLayout, nullptr);
	}

	VkPipeline createComputePipeline(VkPipelineLayout *layout) {
		VkPipeline pipeline;

		auto compShaderCode = IOUtil::readFile("shaders/reactiondiffusion.comp.spv");

		VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = compShaderModule;
		compShaderStageInfo.pName = "main";

		std::array<VkDescriptorSetLayoutBinding, 4> uboLayoutBindings;
		/*	*/
		uboLayoutBindings[0].binding = 0;
		uboLayoutBindings[0].descriptorCount = 1;
		uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uboLayoutBindings[0].pImmutableSamplers = nullptr;
		uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[1].binding = 1;
		uboLayoutBindings[1].descriptorCount = 1;
		uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uboLayoutBindings[1].pImmutableSamplers = nullptr;
		uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[2].binding = 2;
		uboLayoutBindings[2].descriptorCount = 1;
		uboLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		uboLayoutBindings[2].pImmutableSamplers = nullptr;
		uboLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[3].binding = 3;
		uboLayoutBindings[3].descriptorCount = 1;
		uboLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBindings[3].pImmutableSamplers = nullptr;
		uboLayoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VKHelper::createDescriptorSetLayout(getDevice(), descriptorSetLayout, uboLayoutBindings);

		VKHelper::createPipelineLayout(getDevice(), *layout, {descriptorSetLayout});

		VkComputePipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stage = compShaderStageInfo;
		pipelineCreateInfo.layout = *layout;

		VKS_VALIDATE(vkCreateComputePipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline));
		vkDestroyShaderModule(getDevice(), compShaderModule, nullptr);

		return pipeline;
	}

	virtual void Initialize() override {

		// TODO fix memory aligment and physical device.
		paramMemSize = std::max(
			getVKDevice()->getPhysicalDevices()[0]->getDeviceLimits().minUniformBufferOffsetAlignment, paramMemSize);

		/*	Create pipeline.	*/
		computePipeline = createComputePipeline(&computePipelineLayout);

		/*	Create params.	*/
		VkDeviceSize bufferSize = paramMemSize * getSwapChainImageCount();
		paramMemory.resize(1);
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

		for (size_t i = 0; i < paramMemory.size(); i++) {
			VKHelper::createBuffer(getDevice(), bufferSize, memProperties,
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								   paramBuffer, paramMemory[i]);
		}

		/*	Allocate descriptor set.	*/
		std::vector<VkDescriptorPoolSize> poolSize = {{
														  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
														  static_cast<uint32_t>(getSwapChainImageCount() * 1),
													  },

													  {
														  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
														  static_cast<uint32_t>(getSwapChainImageCount()),
													  },
													  {
														  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
														  static_cast<uint32_t>(getSwapChainImageCount()),
													  }};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSize.size();
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.maxSets = static_cast<uint32_t>(getSwapChainImageCount() * 4);

		vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descpool);

		/*	Update params.	*/
		params.kernelA[0][0] = 0.1;
		params.kernelA[0][1] = 0.1;
		params.kernelA[0][2] = 0.1;
		params.kernelA[0][3] = 0.1;

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

		const VkDeviceSize cellBufferSize = (unsigned int)width * (unsigned int)height * sizeof(float) * 1;

		cellsBuffers.resize(getSwapChainImageCount() * 2);
		cellsMemory.resize(getSwapChainImageCount() * 2);
		for (unsigned int i = 0; i < cellsBuffers.size(); i++) {
			VKHelper::createBuffer(getDevice(), cellBufferSize,
								   getVKDevice()->getPhysicalDevices()[0]->getMemoryProperties(),
								   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cellsBuffers[i], cellsMemory[i]);
		}

		/*	*/
		computeImageViews.resize(getSwapChainImageCount());
		for (unsigned int i = 0; i < computeImageViews.size(); i++) {
			if (computeImageViews[i] != nullptr)
				vkDestroyImageView(getDevice(), computeImageViews[i], nullptr);
			computeImageViews[i] =
				VKHelper::createImageView(getDevice(), getSwapChainImages()[i], VK_IMAGE_VIEW_TYPE_2D,
										  getDefaultImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		/*	*/
		vkResetDescriptorPool(getDevice(), descpool, 0);

		std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = descpool; // pool to allocate from.
		descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(getSwapChainImageCount());
		descriptorSetAllocateInfo.pSetLayouts = layouts.data();

		// allocate descriptor set.
		descriptorSets.resize(getSwapChainImageCount());
		VKS_VALIDATE(vkAllocateDescriptorSets(getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

		for (size_t i = 0; i < descriptorSets.size(); i++) {

			VkDescriptorBufferInfo currentCellBufferInfo{};
			currentCellBufferInfo.buffer = cellsBuffers[i * 1 + 0];
			currentCellBufferInfo.offset = 0;
			currentCellBufferInfo.range = cellBufferSize;

			VkDescriptorBufferInfo previousCellBufferInfo{};
			previousCellBufferInfo.buffer = cellsBuffers[i * 1 + 1];
			previousCellBufferInfo.offset = 0;
			previousCellBufferInfo.range = cellBufferSize;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = computeImageViews[i];
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkDescriptorBufferInfo paramBufferInfo{};
			paramBufferInfo.buffer = paramBuffer;
			paramBufferInfo.offset = paramMemSize * i;
			paramBufferInfo.range = paramMemSize;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = nullptr;
			descriptorWrites[0].pBufferInfo = &currentCellBufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = nullptr;
			descriptorWrites[1].pBufferInfo = &previousCellBufferInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &imageInfo;
			descriptorWrites[2].pBufferInfo = nullptr;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = nullptr;
			descriptorWrites[3].pBufferInfo = &paramBufferInfo;

			vkUpdateDescriptorSets(getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		for (size_t i = 0; i < getSwapChainImageCount(); i++) {
			void *data;
			VKS_VALIDATE(vkMapMemory(getDevice(), paramMemory[0], paramMemSize * i, paramMemSize, 0, &data));
			memcpy(data, &params, paramMemSize);
			vkUnmapMemory(getDevice(), paramMemory[0]);
		}

		for (unsigned int i = 0; i < getNrCommandBuffers(); i++) {
			VkCommandBuffer cmd = getCommandBuffers(i);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1,
									&descriptorSets[i], 0, nullptr);

			const int localInvokation = 16;

			vkCmdDispatch(cmd, std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.image = getSwapChainImages()[i];
			imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
								 nullptr, 0, nullptr, 1, &imageMemoryBarrier);

			VKHelper::transitionImageLayout(cmd, gameOfLifeImage[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkImageBlit blitRegion{};
			blitRegion.srcOffsets[1].x = width;
			blitRegion.srcOffsets[1].y = height;
			blitRegion.srcOffsets[1].z = 1;
			blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcSubresource.mipLevel = 0;
			blitRegion.dstOffsets[1].x = width;
			blitRegion.dstOffsets[1].y = height;
			blitRegion.dstOffsets[1].z = 1;
			blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstSubresource.mipLevel = 0;

			vkCmdBlitImage(cmd, gameOfLifeImage[i], VK_IMAGE_LAYOUT_GENERAL, getSwapChainImages()[i],
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);
			VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw() override {
		// Setup the range
		void *data;
		VKS_VALIDATE(
			vkMapMemory(getDevice(), paramMemory[0], paramMemSize * getCurrentFrameIndex(), paramMemSize, 0, &data));
		memcpy(data, &params, paramMemSize);
		vkUnmapMemory(getDevice(), paramMemory[0]);

		int x, y;
		SDL_GetMouseState(&x, &y);
		// params.mousePosX = x;
		// params.mousePosY = y;
		// params.posX = 0;
		// params.posY = 0;
		// params.zoom = 1.0f;
		// params.nrSamples = 128;
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, true},
																		   {"VK_KHR_xlib_surface", true}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, true}};

	try {
		VKSampleWindow<GameOfLife> sample(argc, argv, required_device_extensions, {}, required_instance_extensions);

		sample.run();
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}