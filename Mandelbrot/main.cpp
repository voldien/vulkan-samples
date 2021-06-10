#include "VKHelper.h"
#include "VksCommon.h"
#include "common.hpp"
#include <SDL2/SDL.h>
#include <VKWindow.h>
#include <glm/glm.hpp>

class MandelBrotWindow : public VKWindow {
  private:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout graphicPipelineLayout = VK_NULL_HANDLE;
	VkPipeline computePipeline = VK_NULL_HANDLE;
	VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
	std::vector<VkImageView> computeImageViews;

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool;
	//VkDescriptorSet descriptorSet;

	std::vector<VkDescriptorSet> descriptorSets;
	struct mandelbrot_param_t {
		int windowWidth = 1;
		int windowHeight = 1;
		float posX, posY;
		float mousePosX, mousePosY;
		float zoom; /*  */
		float c;	/*  */
		int nrSamples;
	}params;

  public:
	MandelBrotWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		//	this->setTitle(std::string("Triangle"));
	}
	~MandelBrotWindow(void) {}

	virtual void Release(void) override {
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
		vkFreeMemory(getDevice(), vertexMemory, nullptr);

		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), graphicPipelineLayout, nullptr);

		vkDestroyPipeline(getDevice(), computePipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), computePipelineLayout, nullptr);
	}

	VkPipeline createComputePipeline(VkPipelineLayout *layout) {
		VkPipeline pipeline;

		auto compShaderCode = IOUtil::readFile("shaders/mandelbrot.comp.spv");

		VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = compShaderModule;
		compShaderStageInfo.pName = "main";

		/*	*/
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


		VKHelper::createDescriptorSetLayout(getDevice(), descriptorSetLayout, {uboLayoutBinding});

		VkPushConstantRange pushRange = {
			.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .offset = 0, .size = sizeof(struct mandelbrot_param_t)};
		VKHelper::createPipelineLayout(getDevice(), *layout, {descriptorSetLayout}, {pushRange});

		VkComputePipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stage = compShaderStageInfo;
		pipelineCreateInfo.layout = *layout;

		VK_CHECK(vkCreateComputePipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline));
		return pipeline;
	}

	virtual void Initialize(void) override {
		/*	Create pipeline.	*/
		computePipeline = createComputePipeline(&computePipelineLayout);
		// graphicsPipeline = createGraphicPipeline(&graphicPipelineLayout);

		computeImageViews.resize(swapChainImageCount());
		for (int i = 0;i < computeImageViews.size(); i++) {
			computeImageViews[i] = VKHelper::createImageView(getDevice(), getSwapChainImages()[i], VK_IMAGE_VIEW_TYPE_2D,
															 getDefaultImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		/*	Allocate descriptor set.	*/
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = static_cast<uint32_t>(swapChainImageCount());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImageCount());

		vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descpool);

		std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = descpool; // pool to allocate from.
		descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImageCount());
		descriptorSetAllocateInfo.pSetLayouts = layouts.data();


		// allocate descriptor set.
		descriptorSets.resize(swapChainImageCount());
		VK_CHECK(
			vkAllocateDescriptorSets(getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

		for (int i = 0; i < descriptorSets.size(); i++) {
			VkDescriptorImageInfo bufferInfo{};
			bufferInfo.imageView = computeImageViews[i];
			// imageView

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &bufferInfo;

			vkUpdateDescriptorSets(getDevice(), 1, &descriptorWrite, 0, nullptr);
		}

			onResize(width(), height());
		}

	virtual void onResize(int width, int height) override {

		VK_CHECK(vkQueueWaitIdle(getDefaultGraphicQueue()));

		params.windowWidth = width;
		params.windowHeight = height;

		for (int i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = getDefaultRenderPass();
			renderPassInfo.framebuffer = getFrameBuffers()[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent.width = width;
			renderPassInfo.renderArea.extent.height = height;

			VkClearValue clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &descriptorSets[i], 0,
									NULL);
			/*	*/
			// vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			const int localInvokation = 16;

			vkCmdDispatch(cmd, std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.image = getDefaultImage();
			imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			// Start when the command is executed, finish when compute shader finished writing (?)
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
								 nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			vkCmdPushConstants(cmd, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);

			vkCmdEndRenderPass(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));
		}
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {

	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(argc, argv);
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		printf("%s\n", devices[0]->getDeviceName());
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);

		MandelBrotWindow window(core, d);

		window.run();
	} catch (std::exception &ex) {
		// std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}