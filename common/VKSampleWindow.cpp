#include "VKSampleWindow.h"
#include <map>

VKSampleWindow::VKSampleWindow(int argc, const char **argv,
							   std::unordered_map<const char *, bool> required_device_extensions,
							   std::unordered_map<const char *, bool> required_instance_layers,
							   std::unordered_map<const char *, bool> required_instance_extensions) {

	std::unordered_map<const char *, bool> use_required_device_extensions = {};
	std::unordered_map<const char *, bool> use_required_instance_layers = {};
	std::unordered_map<const char *, bool> use_required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, true},
																			   {"VK_KHR_xlib_surface", true}};

	// TODO append to device extension.
	std::vector<const char *> required_extension = VKWindow::getRequiredDeviceExtensions();
	for (auto it = required_extension.cbegin(); it != required_extension.cend(); it++) {
		required_device_extensions[(*it)] = true;
	}

	std::shared_ptr<VulkanCore> core =
		std::make_shared<VulkanCore>(required_instance_layers, required_instance_extensions);

	std::vector<std::shared_ptr<PhysicalDevice>> physical_devices = core->createPhysicalDevices();

	std::shared_ptr<VKDevice> device = std::make_shared<VKDevice>(physical_devices);
}