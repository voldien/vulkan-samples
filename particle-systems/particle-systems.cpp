#include <VKWindow.h>
#include <stdexcept>

class ParticleSystemWindow : public VKWindow {
  private:
	VkPipeline particleSim = VK_NULL_HANDLE;
	VkPipelineLayout particleSimLayout = VK_NULL_HANDLE;
	VkPipeline particleGraphicPipeline = VK_NULL_HANDLE;
	VkPipelineLayout particleGraphicLayout = VK_NULL_HANDLE;

	/*	*/
	std::vector<VkBuffer> particleBuffers;
	std::vector<VkDeviceMemory> particleBufferMemory;

	/*	*/
	VkBuffer vectorFieldBuffer;
	VkDeviceMemory vectorFieldMemory;

	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void *> mapMemory;

	const int nrParticles = 1000;
	typedef struct particle_t {
		float x, y;		  /*	Position.	*/
		float xdir, ydir; /*	Velocity.	*/
	} alignas(16) Particle;

  public:
	ParticleSystemWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		setTitle("Particle System");
	}

	virtual void Initialize(void) { /*	*/



		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {
		for (uint32_t i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = getDefaultRenderPass();
			renderPassInfo.framebuffer = getFrameBuffers()[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent.width = width;
			renderPassInfo.renderArea.extent.height = height;

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
			clearValues[1].depthStencil = {1.0f, 0};
			renderPassInfo.clearValueCount = clearValues.size();
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = {
				.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			// Bind the compute pipeline.
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicPipeline);

			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(cmd, 0, 1, &particleBuffers[i], offsets);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicLayout, 0, 1,
									&descriptorSets[i], 0, nullptr);

			vkCmdDraw(cmd, nrParticles, 1, 0, 0);

			vkCmdEndRenderPass(cmd);
			// memoryBarrier(cmd, 0, 0, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			// //Bind descriptor
			// set.vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			// 											computePipeline.pipelineLayout, 0, 1,
			// 											&computePipeline.descriptorSet, 0, nullptr);
			// // Dispatch compute job.
			// vkCmdDispatch(cmd, (positionBuffer.size / sizeof(vec2)) / NUM_PARTICLES_PER_WORKGROUP, 1, 1);

			// // Barrier between compute and vertex
			// shading.memoryBarrier(
			// 	cmd, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			// 	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}
};

int main(int argc, const char **argv) {

	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);
		ParticleSystemWindow window(core, d);

		window.run();
	} catch (std::exception &ex) {
		// std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}