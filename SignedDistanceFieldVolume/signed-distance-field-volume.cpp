#include "common.hpp"
#include <VKWindow.h>
#include <stdexcept>

class SignedDistanceFieldVolumeWindow : public VKWindow {
  private:
	VkPipeline particleSim;
	VkPipeline particleGraphicPipeline;

  public:
	SignedDistanceFieldVolumeWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {}

	virtual void Initialize(void) { /*	*/
	
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
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);
		SignedDistanceFieldVolumeWindow window(core, d);

		window.run();
	} catch (std::exception &ex) {
		// std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}