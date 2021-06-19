#include "common.hpp"
#include <Importer/ImageImport.h>
#include <VKWindow.h>
#include <stdexcept>

class SignedDistanceFieldTextureWindow : public VKWindow {
  private:
	VkPipeline particleSim;
	VkPipeline particleGraphicPipeline;
	VkImage texture;
	VkDeviceMemory textureMemory;

  public:
	SignedDistanceFieldTextureWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {}

	virtual void Initialize(void) { /*	*/
		// VkCommandBuffer cmd;
		// std::vector<VkCommandBuffer> cmds =
		// 	this->getLogicalDevice()->beginSingleTimeCommands(this->getGraphicCommandPool());
		// ImageImporter::createImage("/home/voldie/test.png", getDevice(), cmds[0], physicalDevice(), texture,
		// 						   textureMemory);

		VkCommandPool computePool =
			this->getLogicalDevice()->createCommandPool(this->getLogicalDevice()->getDefaultComputeQueueIndex());
		// std::vector<VkCommandBuffer> computeCmds = this->getLogicalDevice()->beginSingleTimeCommands(computePool);

		// getLogicalDevice()->submitCommands(getDefaultGraphicQueue(), cmds);

		// /*	Create compute pipeline to create image.	*/
		// VkCommandBufferBeginInfo beginInfo = {};
		// beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// beginInfo.flags = 0;

		// VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

		// vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

		// vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &descriptorSets[i],
		// 0, 						NULL);

		// const int localInvokation = 16;

		// vkCmdDispatch(cmd, std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

		// VkImageMemoryBarrier imageMemoryBarrier = {};
		// imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		// imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		// imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// imageMemoryBarrier.image = getSwapChainImages()[i];
		// imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		// imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		// imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		// vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
		// 					 nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		// vkEndCommandBuffer(cmd);
		// getLogicalDevice()->submitCommands()
	}

	virtual void onResize(int width, int height) override {
		for (uint32_t i = 0; i < getCommandBuffers().size(); i++) {
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

			VkClearValue clearColor = {0.0f, 1.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// memoryBarrier(cmd, 0, 0, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			// Bind the compute pipeline.
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, particleSim);
			// Bind descriptor set.
			// vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 0, 1,
			// 						&computePipeline.descriptorSet, 0, nullptr);
			// // Dispatch compute job.
			// vkCmdDispatch(cmd, (positionBuffer.size / sizeof(vec2)) / NUM_PARTICLES_PER_WORKGROUP, 1, 1);

			// Barrier between compute and vertex shading.
			// memoryBarrier(cmd, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			// 			  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

			vkCmdEndRenderPass(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));
		}
	}
};

int main(int argc, const char **argv) {

	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(argc, argv);
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);
		SignedDistanceFieldTextureWindow window(core, d);

		window.run();
	} catch (std::exception &ex) {
		// std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}