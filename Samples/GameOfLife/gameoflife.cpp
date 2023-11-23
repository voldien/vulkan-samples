#include <VKWindow.h>
#include <VksCommon.h>
#include <glm/glm.hpp>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class GameOfLife : public VKWindow {
	  private:
		VkPipeline computePipeline = VK_NULL_HANDLE;
		VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

		std::vector<VkImage> gameoflifeRenderImage;
		std::vector<VkDeviceMemory> gameoflifeRenderImageMemory;
		std::vector<VkImageView> computeRenderImageViews;

		std::vector<VkImage> gameoflifeCellImage;
		std::vector<VkDeviceMemory> gameoflifeCellImageMemory;
		std::vector<VkImageView> computeCellImageViews;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descpool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> descriptorSets;
		VkCommandPool computeCmdPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> computeCmds;

		const std::string computeGameOfLifeShaderPath = "shaders/gameoflife/gameoflife.comp.spv";

	  public:
		GameOfLife(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKWindow(core, device, -1, -1, -1, -1) {
			this->setTitle(std::string("Game Of Life"));

			this->show();
		}

		virtual void release() override {
			/*	*/
			vkDestroyCommandPool(this->getDevice(), this->computeCmdPool, nullptr);

			/*	*/
			VKS_VALIDATE(
				vkFreeDescriptorSets(this->getDevice(), descpool, descriptorSets.size(), descriptorSets.data()));
			vkDestroyDescriptorPool(this->getDevice(), descpool, nullptr);
			vkDestroyDescriptorSetLayout(this->getDevice(), descriptorSetLayout, nullptr);

			/*	*/
			for (size_t i = 0; i < this->computeRenderImageViews.size(); i++) {
				vkDestroyImageView(this->getDevice(), this->computeRenderImageViews[i], nullptr);
				vkDestroyImage(this->getDevice(), this->gameoflifeRenderImage[i], nullptr);
				vkFreeMemory(this->getDevice(), this->gameoflifeRenderImageMemory[i], nullptr);
			}

			/*	*/
			for (size_t i = 0; i < computeCellImageViews.size(); i++) {
				vkDestroyImageView(this->getDevice(), this->computeCellImageViews[i], nullptr);
				vkDestroyImage(this->getDevice(), this->gameoflifeCellImage[i], nullptr);
				vkFreeMemory(this->getDevice(), this->gameoflifeCellImageMemory[i], nullptr);
			}

			/*	*/
			vkDestroyPipeline(this->getDevice(), this->computePipeline, nullptr);
			vkDestroyPipelineLayout(this->getDevice(), this->computePipelineLayout, nullptr);
		}

		VkPipeline createComputePipeline(VkPipelineLayout *layout) {
			VkPipeline pipeline;

			auto compShaderCode =
				vksample::IOUtil::readFileData<uint32_t>(this->computeGameOfLifeShaderPath, this->getFileSystem());

			VkShaderModule compShaderModule = VKHelper::createShaderModule(this->getDevice(), compShaderCode);

			VkPipelineShaderStageCreateInfo compShaderStageInfo{};
			compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			compShaderStageInfo.module = compShaderModule;
			compShaderStageInfo.pName = "main";

			std::array<VkDescriptorSetLayoutBinding, 3> uboLayoutBindings;
			/*	Previous Cell.	*/
			uboLayoutBindings[0].binding = 0;
			uboLayoutBindings[0].descriptorCount = 1;
			uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uboLayoutBindings[0].pImmutableSamplers = nullptr;
			uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			/*	Current Cell.	*/
			uboLayoutBindings[1].binding = 1;
			uboLayoutBindings[1].descriptorCount = 1;
			uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uboLayoutBindings[1].pImmutableSamplers = nullptr;
			uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			/*	Render Texture.	*/
			uboLayoutBindings[2].binding = 2;
			uboLayoutBindings[2].descriptorCount = 1;
			uboLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			uboLayoutBindings[2].pImmutableSamplers = nullptr;
			uboLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			/*	*/
			VKHelper::createDescriptorSetLayout(getDevice(), descriptorSetLayout, uboLayoutBindings);

			/*	*/
			VKHelper::createPipelineLayout(getDevice(), *layout, {descriptorSetLayout});

			pipeline = VKHelper::createComputePipeline(getDevice(), *layout, compShaderStageInfo);

			vkDestroyShaderModule(getDevice(), compShaderModule, nullptr);

			return pipeline;
		}

		virtual void Initialize() override {

			/*	Create pipeline.	*/
			this->computePipeline = createComputePipeline(&computePipelineLayout);

			/*	Allocate descriptor set.	*/
			const std::vector<VkDescriptorPoolSize> poolSize = {{
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				static_cast<uint32_t>(getSwapChainImageCount() * 3), /*	3 storage image for each chain.	*/
			}};

			/*	*/
			this->descpool = VKHelper::createDescPool(getDevice(), poolSize, this->getSwapChainImageCount() * 3);

			/*	Create game of life render image.	*/
			this->gameoflifeRenderImage.resize(this->getSwapChainImageCount());
			this->gameoflifeRenderImageMemory.resize(this->getSwapChainImageCount());

			/*	Create game of life cells.	previous and current.	*/
			this->gameoflifeCellImage.resize(2);
			this->gameoflifeCellImageMemory.resize(2);

			onResize(width(), height());
		}

		virtual void onResize(int width, int height) override {

			/*	Wait in till the resources are not used.	*/
			VKS_VALIDATE(vkQueueWaitIdle(this->getDefaultGraphicQueue()));

			/*	Create render images.	*/
			for (size_t i = 0; i < this->gameoflifeRenderImageMemory.size(); i++) {
				if (this->gameoflifeRenderImage[i] != nullptr) {
					vkDestroyImage(this->getDevice(), this->gameoflifeRenderImage[i], nullptr);
				}
				VKHelper::createImage(
					this->getDevice(), this->width(), this->height(), 1, VK_FORMAT_R8G8B8A8_UNORM,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(),
					this->gameoflifeRenderImage[i], this->gameoflifeRenderImageMemory[i]);
			}

			/*	Create cell state image.	*/
			for (size_t i = 0; i < this->gameoflifeCellImage.size(); i++) {
				if (gameoflifeCellImage[i] != nullptr) {
					vkDestroyImage(this->getDevice(), this->gameoflifeCellImage[i], nullptr);
				}
				VKHelper::createImage(
					this->getDevice(), this->width(), this->height(), 1, VK_FORMAT_R8_UINT, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					this->getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(), this->gameoflifeCellImage[i],
					this->gameoflifeCellImageMemory[i]);
			}

			// Upload init random data.
			{
				// TODO create the default buffer.
				std::vector<uint8_t> textureData(width * height * sizeof(uint8_t));

				/*	Generate random game state.	*/
				for (int j = 0; j < height; j++) {
					for (int i = 0; i < width; i++) {
						/*	Random value between dead and alive cells.	*/
						textureData[width * j + i] = fragcore::Random::range(0, 2);
					}
				}

				/*	*/
				VkCommandPool commandPool = this->getGraphicCommandPool();
				VkCommandBuffer cmd = VKHelper::beginSingleTimeCommands(this->getDevice(), commandPool);
				VkBuffer stagingBuffer;
				VkDeviceMemory stagingBufferMemory;

				VkPhysicalDeviceMemoryProperties memProperties;

				vkGetPhysicalDeviceMemoryProperties(this->getVKDevice()->getPhysicalDevice(0)->getHandle(),
													&memProperties);

				VKHelper::createBuffer(this->getDevice(), textureData.size(), memProperties,
									   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
									   stagingBuffer, stagingBufferMemory);

				void *stageData;
				vkMapMemory(this->getDevice(), stagingBufferMemory, 0, textureData.size(), 0, &stageData);
				memcpy(stageData, textureData.data(), static_cast<size_t>(textureData.size()));
				vkUnmapMemory(this->getDevice(), stagingBufferMemory);

				VKHelper::copyBufferToImageCmd(
					cmd, stagingBuffer, this->gameoflifeCellImage[0],
					{static_cast<uint32_t>(this->width()), static_cast<uint32_t>(this->height()), 1});
				// for (size_t i = 0; i < this->gameoflifeRenderImage.size(); i++) {
				//	VKHelper::transitionImageLayout(cmd, this->gameoflifeRenderImage[i], VK_IMAGE_LAYOUT_UNDEFINED,
				//									VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				//}

				VKHelper::endSingleTimeCommands(this->getDevice(), this->getDefaultTransferQueue(), cmd, commandPool);

				vkDestroyBuffer(this->getDevice(), stagingBuffer, nullptr);
				vkFreeMemory(this->getDevice(), stagingBufferMemory, nullptr);
			}

			/*	*/
			this->computeRenderImageViews.resize(this->getSwapChainImageCount());
			for (size_t i = 0; i < this->computeRenderImageViews.size(); i++) {
				if (this->computeRenderImageViews[i] != nullptr) {
					vkDestroyImageView(this->getDevice(), this->computeRenderImageViews[i], nullptr);
				}
				/*	*/
				computeRenderImageViews[i] =
					VKHelper::createImageView(this->getDevice(), gameoflifeRenderImage[i], VK_IMAGE_VIEW_TYPE_2D,
											  this->getDefaultImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
			}

			this->computeCellImageViews.resize(this->gameoflifeCellImage.size());
			for (size_t i = 0; i < gameoflifeCellImage.size(); i++) {
				if (computeCellImageViews[i] != nullptr) {
					vkDestroyImageView(getDevice(), computeCellImageViews[i], nullptr);
				}
				/*	*/
				computeCellImageViews[i] =
					VKHelper::createImageView(this->getDevice(), this->gameoflifeCellImage[i], VK_IMAGE_VIEW_TYPE_2D,
											  VK_FORMAT_R8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			}

			/*	*/
			VKS_VALIDATE(vkResetDescriptorPool(this->getDevice(), descpool, 0));

			std::vector<VkDescriptorSetLayout> layouts(this->getSwapChainImageCount(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = descpool; // pool to allocate from.
			descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(this->getSwapChainImageCount());
			descriptorSetAllocateInfo.pSetLayouts = layouts.data();

			// allocate descriptor set.
			descriptorSets.resize(this->getSwapChainImageCount());
			// vkFreeDescriptorSets
			VKS_VALIDATE(
				vkAllocateDescriptorSets(this->getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

			for (size_t i = 0; i < descriptorSets.size(); i++) {
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageView = computeRenderImageViews[i];
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				VkDescriptorImageInfo imagePreviousCellInfo{};
				imagePreviousCellInfo.imageView = computeCellImageViews[(i + 0) % this->computeCellImageViews.size()];
				imagePreviousCellInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				VkDescriptorImageInfo imagCurrentCellInfo{};
				imagCurrentCellInfo.imageView = computeCellImageViews[(i + 1) % this->computeCellImageViews.size()];
				imagCurrentCellInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = descriptorSets[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = &imagePreviousCellInfo;
				descriptorWrites[0].pBufferInfo = nullptr;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = descriptorSets[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &imagCurrentCellInfo;
				descriptorWrites[1].pBufferInfo = nullptr;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = descriptorSets[i];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = &imageInfo;
				descriptorWrites[2].pBufferInfo = nullptr;

				vkUpdateDescriptorSets(this->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			}

			// TODO resolve for if compute queue is not part of graphic queue.
			/*	Create command queue.	*/
			for (size_t i = 0; i < this->getNrCommandBuffers(); i++) {
				VkCommandBuffer cmd = this->getCommandBuffers(i);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				/*	Sync */

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipeline);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipelineLayout, 0, 1,
										&descriptorSets[i], 0, nullptr);

				const float localInvokation = 8; // TODO fetch

				vkCmdDispatch(cmd, std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

				/*	*/
				std::vector<VkImageMemoryBarrier> dispatchBarrier(2);
				dispatchBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				dispatchBarrier[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				dispatchBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				dispatchBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[0].image = this->gameoflifeRenderImage[i];
				dispatchBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				dispatchBarrier[0].subresourceRange.baseMipLevel = 0;
				dispatchBarrier[0].subresourceRange.levelCount = 1;
				dispatchBarrier[0].subresourceRange.baseArrayLayer = 0;
				dispatchBarrier[0].subresourceRange.layerCount = 1;
				dispatchBarrier[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				dispatchBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				dispatchBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				dispatchBarrier[1].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				dispatchBarrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				dispatchBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[1].image = this->getSwapChainImages()[i];
				dispatchBarrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				dispatchBarrier[1].subresourceRange.baseMipLevel = 0;
				dispatchBarrier[1].subresourceRange.levelCount = 1;
				dispatchBarrier[1].subresourceRange.baseArrayLayer = 0;
				dispatchBarrier[1].subresourceRange.layerCount = 1;
				dispatchBarrier[1].srcAccessMask = 0;
				dispatchBarrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
									 nullptr, 0, nullptr, 2, dispatchBarrier.data());

				/*	*/
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

				/*	*/
				vkCmdBlitImage(cmd, this->gameoflifeRenderImage[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							   this->getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
							   VK_FILTER_NEAREST);

				/*	*/
				std::vector<VkImageMemoryBarrier> blitBarriers(2);
				blitBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				blitBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				blitBarriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				blitBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				blitBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				blitBarriers[0].image = this->getSwapChainImages()[i];
				blitBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitBarriers[0].subresourceRange.baseMipLevel = 0;
				blitBarriers[0].subresourceRange.levelCount = 1;
				blitBarriers[0].subresourceRange.baseArrayLayer = 0;
				blitBarriers[0].subresourceRange.layerCount = 1;
				blitBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				blitBarriers[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

				blitBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				blitBarriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				blitBarriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				blitBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				blitBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				blitBarriers[1].image = this->gameoflifeRenderImage[i];
				blitBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitBarriers[1].subresourceRange.baseMipLevel = 0;
				blitBarriers[1].subresourceRange.levelCount = 1;
				blitBarriers[1].subresourceRange.baseArrayLayer = 0;
				blitBarriers[1].subresourceRange.layerCount = 1;
				blitBarriers[1].srcAccessMask = 0;
				blitBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
									 0, nullptr, blitBarriers.size(), blitBarriers.data());

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}
		}

		virtual void draw() override {
			// Setup the range
		}
	};
} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		VKSample<vksample::GameOfLife> sample;
		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}