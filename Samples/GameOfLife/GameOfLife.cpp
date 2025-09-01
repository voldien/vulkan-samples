#include "Util/ShaderLoader.h"
#include "VKDataStructure.h"
#include "VKSample.h"
#include "vulkan/vulkan_core.h"
#include <VKWindow.h>
#include <cstddef>
#include <glm/glm.hpp>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class GameOfLife : public VKBaseSampleWindow {
	  private:
		ComputePipeline computePipeline;
		std::array<size_t, 3> localSize{};

		std::vector<VkImage> gameoflifeRenderImage;
		std::vector<VkDeviceMemory> gameoflifeRenderImageMemory;
		std::vector<VkImageView> computeRenderImageViews;

		std::vector<VkImage> gameoflifeCellImage;
		std::vector<VkDeviceMemory> gameoflifeCellImageMemory;
		std::vector<VkImageView> computeCellImageViews;

		std::vector<VkDescriptorSet> descriptorSets;
		VkCommandPool computeCmdPool = VK_NULL_HANDLE;

		const std::string computeGameOfLifeShaderPath = "Shaders/gameoflife/gameoflife.comp.spv";

	  public:
		GameOfLife(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle(std::string("Game Of Life"));

			this->show();
		}

		void release() override {
			/*	*/
			vkDestroyCommandPool(this->getDevice(), this->computeCmdPool, nullptr);

			/*	*/
			VKS_VALIDATE(vkFreeDescriptorSets(this->getDevice(), getDescriptorPool(), descriptorSets.size(),
											  descriptorSets.data()));

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
		}

		void Initialize() override {

			/*	Create pipeline.	*/
			ShaderLoader loader(*this);

			const std::vector<uint32_t> compShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->computeGameOfLifeShaderPath, this->getFileSystem());

			this->computePipeline = loader.loadComputeProgram(&compShaderCode);
			this->localSize = PipelineLayoutUtil::getLocalSize(compShaderCode).value();

			/*	Create game of life render image.	*/
			this->gameoflifeRenderImage.resize(this->getSwapChainImageCount());
			this->gameoflifeRenderImageMemory.resize(this->getSwapChainImageCount());

			/*	Create game of life cells.	previous and current.	*/
			this->gameoflifeCellImage.resize(2);
			this->gameoflifeCellImageMemory.resize(2);

			onResize(width(), height());
		}

		void onResize(int width, int height) override {

			/*	Wait in till the resources are not used.	*/
			VKS_VALIDATE(vkQueueWaitIdle(this->getDefaultGraphicQueue()));

			/*	Create render images.	*/
			for (size_t i = 0; i < this->gameoflifeRenderImageMemory.size(); i++) {
				if (this->gameoflifeRenderImage[i] != nullptr) {
					vkDestroyImage(this->getDevice(), this->gameoflifeRenderImage[i], this->getAllocatorCallback());
				}

				/*	*/
				VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
				VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
				VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				this->allocateImage(this->width(), this->height(), 1, imageFormat, tiling, usage, memFlags,
									VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, this->gameoflifeRenderImage[i],
									this->gameoflifeRenderImageMemory[i]);
			}

			/*	Create cell state image.	*/
			for (size_t i = 0; i < this->gameoflifeCellImage.size(); i++) {
				if (gameoflifeCellImage[i] != nullptr) {
					vkDestroyImage(this->getDevice(), this->gameoflifeCellImage[i], nullptr);
				}
				VkImageUsageFlags usage =
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				VkFormat imageFormat = VK_FORMAT_R8_UINT;
				VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
				VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

				this->allocateImage(this->width(), this->height(), 1, imageFormat, tiling, usage, memFlags,
									VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, this->gameoflifeCellImage[i],
									this->gameoflifeCellImageMemory[i]);
			}

			// Upload init random data.
			{
				// TODO create the default buffer.
				std::vector<uint8_t> textureData(static_cast<unsigned long>(width * height) * sizeof(uint8_t));

				/*	Generate random game state.	*/
				for (int j = 0; j < height; j++) {
					for (int i = 0; i < width; i++) {
						/*	Random value between dead and alive cells.	*/
						textureData[(width * j) + i] = fragcore::Random::range(0, 2);
					}
				}
				this->transferImageData(gameoflifeCellImage[0], gameoflifeCellImageMemory[0], this->width(),
										this->height(), 1, textureData.data(),
										textureData.size() * sizeof(textureData[0]));
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

			std::vector<VkDescriptorSetLayout> layouts(this->getSwapChainImageCount(),
													   this->computePipeline.setLayout[0]);
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.pNext = nullptr;
			descriptorSetAllocateInfo.descriptorPool = getDescriptorPool(); // pool to allocate from.
			descriptorSetAllocateInfo.descriptorSetCount = this->computePipeline.numSetLayout;
			descriptorSetAllocateInfo.pSetLayouts = this->computePipeline.setLayout.data();

			// allocate descriptor set.
			descriptorSets.resize(this->getSwapChainImageCount());
			for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {
				VKS_VALIDATE(
					vkAllocateDescriptorSets(this->getDevice(), &descriptorSetAllocateInfo, &descriptorSets[i]));
			}

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
			for (size_t index = 0; index < this->getNrCommandBuffers(); index++) {
				VkCommandBuffer cmd = this->getCommandBuffers(index);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				/*	Sync */

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipeline.pipeline);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipeline.layout, 0, 1,
										&descriptorSets[index], 0, nullptr);

				const size_t displatchX = std::ceil(width / localSize[0]);
				const size_t displatchY = std::ceil(height / localSize[1]);

				vkCmdDispatch(cmd, displatchX, displatchY, 1);

				/*	Wait in till the dispatch is finished before blitting.	*/
				std::vector<VkImageMemoryBarrier> dispatchBarrier(2);
				dispatchBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				dispatchBarrier[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				dispatchBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				dispatchBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[0].image = this->gameoflifeRenderImage[index];
				dispatchBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				dispatchBarrier[0].subresourceRange.baseMipLevel = 0;
				dispatchBarrier[0].subresourceRange.levelCount = 1;
				dispatchBarrier[0].subresourceRange.baseArrayLayer = 0;
				dispatchBarrier[0].subresourceRange.layerCount = 1;
				dispatchBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				dispatchBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				dispatchBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				dispatchBarrier[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				dispatchBarrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				dispatchBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				dispatchBarrier[1].image = this->getSwapChainImages()[index];
				dispatchBarrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				dispatchBarrier[1].subresourceRange.baseMipLevel = 0;
				dispatchBarrier[1].subresourceRange.levelCount = 1;
				dispatchBarrier[1].subresourceRange.baseArrayLayer = 0;
				dispatchBarrier[1].subresourceRange.layerCount = 1;
				dispatchBarrier[1].srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
				dispatchBarrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
									 nullptr, 0, nullptr, dispatchBarrier.size(), dispatchBarrier.data());

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
				vkCmdBlitImage(cmd, this->gameoflifeRenderImage[index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							   this->getSwapChainImages()[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
							   VK_FILTER_NEAREST);

				/*	*/
				std::vector<VkImageMemoryBarrier> blitBarriers(1);
				blitBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				blitBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				blitBarriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				blitBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				blitBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				blitBarriers[0].image = this->getSwapChainImages()[index];
				blitBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitBarriers[0].subresourceRange.baseMipLevel = 0;
				blitBarriers[0].subresourceRange.levelCount = 1;
				blitBarriers[0].subresourceRange.baseArrayLayer = 0;
				blitBarriers[0].subresourceRange.layerCount = 1;
				blitBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				blitBarriers[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

				// blitBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				// blitBarriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				// blitBarriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				// blitBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				// blitBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				// blitBarriers[1].image = this->gameoflifeRenderImage[i];
				// blitBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				// blitBarriers[1].subresourceRange.baseMipLevel = 0;
				// blitBarriers[1].subresourceRange.levelCount = 1;
				// blitBarriers[1].subresourceRange.baseArrayLayer = 0;
				// blitBarriers[1].subresourceRange.layerCount = 1;
				// blitBarriers[1].srcAccessMask = 0;
				// blitBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0,
									 0, nullptr, 0, nullptr, blitBarriers.size(), blitBarriers.data());

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}
		}

		void draw() override {
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