#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <VKWindow.h>

class StartUpWindow : public VKWindow {
  public:
	StartUpWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {}

	virtual void Initialize(void) override {
		/*	Check if supported.	*/
		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

		for (unsigned int i = 0; i < getNrCommandBuffers(); i++) {
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

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
			clearValues[1].depthStencil = {1.0f, 0};
			renderPassInfo.clearValueCount = clearValues.size();
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdEndRenderPass(cmd);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw(void) override {}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, true}};

	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		printf("%s\n", devices[0]->getDeviceName());
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices, required_device_extensions);
		StartUpWindow window(core, d);

		window.run();
	} catch (std::exception &ex) {
		// std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}