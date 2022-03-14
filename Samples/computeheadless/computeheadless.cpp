#include "FPSCounter.h"
#include "VKSampleWindow.h"
#include "VksCommon.h"
#include <VKWindow.h>
#include <glm/glm.hpp>

class ComputeHeadless : public VKSampleSession {
  public:
	ComputeHeadless(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device) {}

	void prepare_pipeline() {}

	virtual void run() override {
		const int nrTransferSamples = 100;
		prepare_pipeline();
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, false},
																		   {"VK_KHR_xlib_surface", false}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, false}};

	try {
		VKSampleWindow<ComputeHeadless> computeheadless(argc, argv, required_device_extensions, {},
														required_instance_extensions);
		computeheadless.run();

	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}