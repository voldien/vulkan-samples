#include "common.hpp"
#include <VKWindow.h>
#include<stdexcept>
/**
 *
 */
class RayTracing : public VKWindow {
	private:
	  VkAccelerationStructureKHR acc;

	public:
	  RayTracing(std::shared_ptr<VulkanCore> &core) : VKWindow(core, 0, 0, 800, 600) {}

	  virtual void Initialize(void) { /*	*/
		  VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingFeatures = {
			  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
		  };
		  VkPhysicalDeviceFeatures2 features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
												.pNext = &raytracingFeatures};

		  vkGetPhysicalDeviceFeatures2(physicalDevice(), &features);
		  if(!raytracingFeatures.rayTracingPipeline)
			  throw std::runtime_error("RayTracing not supported!");
		  //		  vkGetPhysicalDeviceProperties2(getPhyiscalDevices(), )
	  }
};

int main(int argc, const char **argv) {

	std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(argc, argv);
	RayTracing window(core);

	window.run();
	return EXIT_SUCCESS;
}