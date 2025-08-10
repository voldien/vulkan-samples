#include "VKSample.h"
#include <VKWindow.h>

namespace vksample {

	class StartUpWindow : public VKBaseSampleWindow {
	  public:
		StartUpWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle("StartUp Window");
			this->show();
		}
		~StartUpWindow() override = default;

		void Initialize() override { this->onResize(width(), height()); }

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

			/*	Rebuild the command buffer.	*/
			for (size_t i = 0; i < this->getNrCommandBuffers(); i++) {
				VkCommandBuffer cmd = this->getCommandBuffers(i);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				/*	Setup render pass.	*/
				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = getDefaultRenderPass();
				renderPassInfo.framebuffer = getFrameBuffer(i);
				renderPassInfo.renderArea.offset = {0, 0};
				renderPassInfo.renderArea.extent.width = width;
				renderPassInfo.renderArea.extent.height = height;

				/*	Clear and depth buffer.	*/
				std::array<VkClearValue, 2> clearValues{};
				clearValues[0].color = {1.0f, 1.0f, 1.0f, 1.0f};
				clearValues[1].depthStencil = {1.0f, 0};
				renderPassInfo.clearValueCount = clearValues.size();
				renderPassInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdEndRenderPass(cmd);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}
		}

		void draw() override {}

		void update() override {}
	};
} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {};
	std::unordered_map<const char *, bool> required_instance_extensions = {};

	try {
		VKSample<vksample::StartUpWindow> sample;
		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}