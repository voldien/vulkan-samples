#include "Importer/ImageImport.h"
#include "VKUtil.h"
#include "VksCommon.h"
#include <cxxopts.hpp>
#include <fmt/format.h>

class CachePipeline : public VKSampleSession {
  public:
	MemoryTransfer(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKSampleSession(core, device) {}

	virtual void run() override {


		
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, false},
																		   {"VK_KHR_xlib_surface", false}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, false}};

	try {
		VKSampleWindow<CachePipeline> sample(argc, argv, required_device_extensions, {}, required_instance_extensions);
		sample.run();

	} catch (const std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}