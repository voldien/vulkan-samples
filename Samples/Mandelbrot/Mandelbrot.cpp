#include "Util/PipelineLayoutUtil.h"
#include "Util/ShaderLoader.h"
#include "VKDataStructure.h"
#include "VKSample.h"
#include <SDL_mouse.h>
#include <VKWindow.h>
#include <array>
#include <glm/glm.hpp>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class MandelBrotWindow : public VKBaseSampleWindow {
	  private:
		ComputePipeline computePipeline;
		std::array<size_t, 3> localSize;

		std::vector<VkImage> mandelBrotImage;
		std::vector<VkDeviceMemory> mandelBrotImageMemory;
		std::vector<VkImageView> computeImageViews;

		// TODO merge.
		VkDeviceMemory paramMemory = VK_NULL_HANDLE;
		VkBuffer paramBuffer = VK_NULL_HANDLE;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> descriptorSets;
		VkCommandPool computeCmdPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> computeCmds;

		const std::string computeMandelBrotShaderPath = "Shaders/mandelbrot/mandelbrot.comp.spv";

		struct mandelbrot_param_t {
			float posX{}, posY{};
			float mousePosX{}, mousePosY{};
			float zoom = 1.0f; /*	*/
			float c = 0;	   /*	*/
			float ci = 1;	   /*	*/
			int nrSamples = 128;
		} params = {};

		size_t paramMemSize = sizeof(params);

	  public:
		MandelBrotWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle(std::string("MandelBrot"));
			this->show();
		}
		~MandelBrotWindow() override = default;

		void release() override {
			/*	*/
			vkDestroyCommandPool(getDevice(), this->computeCmdPool, nullptr);

			/*	*/
			VKS_VALIDATE(
				vkFreeDescriptorSets(getDevice(), getDescriptorPool(), descriptorSets.size(), descriptorSets.data()));
			vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);

			for (size_t i = 0; i < computeImageViews.size(); i++) {
				vkDestroyImageView(getDevice(), computeImageViews[i], nullptr);
				vkDestroyImage(getDevice(), mandelBrotImage[i], nullptr);
				vkFreeMemory(getDevice(), mandelBrotImageMemory[i], nullptr);
			}

			vkDestroyBuffer(getDevice(), paramBuffer, nullptr);
			vkFreeMemory(getDevice(), paramMemory, nullptr);

			vkDestroyPipeline(getDevice(), computePipeline.pipeline, nullptr);
			vkDestroyPipelineLayout(getDevice(), computePipeline.layout, nullptr);
		}

		void Initialize() override {

			/*	Create pipeline.	*/
			ShaderLoader loader(*this);

			const std::vector<uint32_t> compShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->computeMandelBrotShaderPath, this->getFileSystem());

			this->computePipeline = loader.loadComputeProgram(&compShaderCode);
			this->localSize = PipelineLayoutUtil::getLocalSize(compShaderCode).value();

			// TODO fix physical device.
			const size_t minMapBufferSize = getPhysicalDevice()->getDeviceLimits().minUniformBufferOffsetAlignment;
			this->paramMemSize = fragcore::Math::align(this->paramMemSize, minMapBufferSize);

			VkDeviceSize bufferSize = paramMemSize * getSwapChainImageCount();

			const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			const VkMemoryPropertyFlags memoryFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |	/*	*/
													 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | /*	*/
													 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	/*	*/
			this->allocateBuffer(bufferSize, usageFlags, memoryFlag, this->paramBuffer, this->paramMemory);

			onResize(width(), height());
		}

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

			/*	Create reaction diffusion image and buffer.	*/
			this->mandelBrotImage.resize(getSwapChainImageCount());
			this->mandelBrotImageMemory.resize(getSwapChainImageCount());
			for (size_t i = 0; i < mandelBrotImageMemory.size(); i++) {

				VKHelper::createImage2D(
					getDevice(), this->width(), this->height(), 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(),
					VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, mandelBrotImage[i], mandelBrotImageMemory[i]);
			}

			/*	*/
			computeImageViews.resize(getSwapChainImageCount());
			for (size_t i = 0; i < computeImageViews.size(); i++) {
				if (computeImageViews[i] != nullptr) {
					vkDestroyImageView(getDevice(), computeImageViews[i], nullptr);
				}
				computeImageViews[i] = VKHelper::createImageView(getDevice(), mandelBrotImage[i], VK_IMAGE_VIEW_TYPE_2D,
																 getDefaultImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
			}

			/*	*/
			VKS_VALIDATE(vkResetDescriptorPool(getDevice(), getDescriptorPool(), 0));

			std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = getDescriptorPool(); // pool to allocate from.
			descriptorSetAllocateInfo.descriptorSetCount = getSwapChainImageCount();
			descriptorSetAllocateInfo.pSetLayouts = layouts.data();

			// allocate descriptor set.
			descriptorSets.resize(getSwapChainImageCount());
			VKS_VALIDATE(vkAllocateDescriptorSets(getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

			for (size_t desc_index = 0; desc_index < descriptorSets.size(); desc_index++) {
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageView = computeImageViews[desc_index];
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = paramBuffer;
				bufferInfo.offset = paramMemSize * desc_index;
				bufferInfo.range = paramMemSize;

				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = descriptorSets[desc_index];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = &imageInfo;
				descriptorWrites[0].pBufferInfo = nullptr;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = descriptorSets[desc_index];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = nullptr;
				descriptorWrites[1].pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			}

			for (size_t i = 0; i < getSwapChainImageCount(); i++) {
				void *data = nullptr;
				VKS_VALIDATE(vkMapMemory(getDevice(), paramMemory, paramMemSize * i, paramMemSize, 0, &data));
				memcpy(data, &params, paramMemSize);
				vkUnmapMemory(getDevice(), paramMemory);
			}

			// TODO resolve for if compute queue is not part of graphic queue.
			for (size_t i = 0; i < getNrCommandBuffers(); i++) {
				VkCommandBuffer cmd = getCommandBuffers(i);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipeline);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.layout, 0, 1,
										&descriptorSets[i], 0, nullptr);

				const unsigned int WorkGroupX = std::ceil(width / (float)localSize[0]);
				const unsigned int WorkGroupY = std::ceil(height / (float)localSize[1]);

				vkCmdDispatch(cmd, WorkGroupX, WorkGroupY, 1);

				VKHelper::imageBarrier(cmd, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
									   getSwapChainImages()[i], VK_IMAGE_LAYOUT_GENERAL,
									   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
									   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				VKHelper::transitionImageLayout(cmd, mandelBrotImage[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

				vkCmdBlitImage(cmd, mandelBrotImage[i], VK_IMAGE_LAYOUT_GENERAL, getSwapChainImages()[i],
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);
				VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

				VKHelper::transitionImageLayout(cmd, mandelBrotImage[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
												VK_IMAGE_LAYOUT_GENERAL);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}
		}

		void draw() override {
			// Setup the range
			void *data = nullptr;
			VKS_VALIDATE(
				vkMapMemory(getDevice(), paramMemory, paramMemSize * getCurrentFrameIndex(), paramMemSize, 0, &data));
			memcpy(data, &params, paramMemSize);
			vkUnmapMemory(getDevice(), paramMemory);

			/*	Update.	*/
			int x = 0, y = 0;
			SDL_GetMouseState(&x, &y);
			params.mousePosX = x;
			params.mousePosY = y;
			params.posX = 0;
			params.posY = 0;
			params.zoom = 1.0f;
			params.nrSamples = 128;
		}
	};

	/*	*/

} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		VKSample<vksample::MandelBrotWindow> sample;
		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}