
#include <VksCommon.h>

/**
 *
 */
class RayTracing : public VKWindow {
  private:
	VkAccelerationStructureKHR acc;

  public:
	RayTracing(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {}
	virtual void Initialize() {
		/*	*/

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingFeatures = {};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR prop;
		getVKDevice()->getPhysicalDevices()[0]->checkFeature(
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, raytracingFeatures);
		getVKDevice()->getPhysicalDevices()[0]->getProperties(
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR, prop);
		if (!raytracingFeatures.rayTracingPipeline) {
			throw cxxexcept::RuntimeException("RayTracing not supported!");
		}

		//		  vkGetPhysicalDeviceProperties2(getPhyiscalDevices(), )

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

		for (uint32_t i = 0; i < getNrCommandBuffers(); i++) {
			VkCommandBuffer cmd = getCommandBuffers(i);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = getDefaultRenderPass();
			renderPassInfo.framebuffer = getFrameBuffer(i);
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent.width = width;
			renderPassInfo.renderArea.extent.height = height;

			VkClearValue clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			VkViewport viewport = {
				.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);
			// vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 0,
			// 						(uint32_t)descSets.size(), descSets.data(), 0, nullptr);
			// vkCmdPushConstants(cmd, m_rtPipelineLayout,
			// 				   VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
			// 					   VK_SHADER_STAGE_MISS_BIT_KHR,
			// 				   0, sizeof(PushConstantRay), &m_pcRay);

			// vkCmdTraceRaysKHR(cmd, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, m_size.width,
			// 				  m_size.height, 1);

			// vkCmdDraw(cmd, vertices.size(), 1, 0, 0);

			vkCmdEndRenderPass(cmd);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, true},
																		   {"VK_KHR_xlib_surface", true}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, true},
																		 {"VK_KHR_ray_tracing_pipeline", true},
																		 {"VK_KHR_acceleration_structure", true}};

	try {
		VKSampleWindow<RayTracing> sample(argc, argv, required_device_extensions, {}, required_instance_extensions);
		sample.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}