
#include <VKWindow.h>
#include <stdexcept>
/**
 *
 */
class RayTracing : public VKWindow {
  private:
	VkAccelerationStructureKHR acc;

  public:
	RayTracing(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {}
	virtual void Initialize(void) {
		/*	*/

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingFeatures = {};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR prop;
		getVKDevice()->getPhysicalDevices()[0]->checkFeature(
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, raytracingFeatures);
		getVKDevice()->getPhysicalDevices()[0]->getProperties(
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR, prop);
		if (!raytracingFeatures.rayTracingPipeline)
			throw cxxexcept::RuntimeException("RayTracing not supported!");
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

			/*	Transfer the new data 	*/
			// vkCmdTran

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

			// /*	*/
			// vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// VkBuffer vertexBuffers[] = {vertexBuffer};
			// VkDeviceSize offsets[] = {0};
			// vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

			// vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i],
			// 0, 						nullptr);

			// vkCmdDraw(cmd, vertices.size(), 1, 0, 0);

			// vkCmdEndRenderPass(cmd);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {{"VK_KHR_ray_tracing_pipeline", true},
																		 {"VK_KHR_acceleration_structure", true}};
	std::unordered_map<const char *, bool> required_instance_layers = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(required_instance_layers);
		std::vector<std::shared_ptr<PhysicalDevice>> p = core->createPhysicalDevices();
		printf("%s\n", p[0]->getDeviceName());
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(p, required_device_extensions);
		RayTracing window(core, d);

		window.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}