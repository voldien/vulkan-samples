#ifndef _VK_SAMPLE_WINDOW_H_
#define _VK_SAMPLE_WINDOW_H_ 1
#include "VKWindow.h"
#include <cxxopts.hpp>
#include <iostream>
#include <map>

class VKSampleSession {
  public:
	VKSampleSession(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: core(core), device(device) {}
	virtual void run() {}
	virtual void commandline(cxxopts::Options &options) {}

	/*	*/
	VkDevice getDevice() const noexcept { return this->device->getHandle(); }

	uint32_t getGraphicQueueIndex() const;
	VkQueue getDefaultGraphicQueue() const;
	VkQueue getDefaultComputeQueue() const;

	const std::shared_ptr<VKDevice> &getVKDevice() const noexcept { return this->device; }
	const std::shared_ptr<PhysicalDevice> getPhysicalDevice() const noexcept;

	VkPhysicalDevice physicalDevice() const;
	void setPhysicalDevice(VkPhysicalDevice device);
	std::vector<VkQueue> getQueues() const noexcept;
	const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const;

  protected:
	std::shared_ptr<VKDevice> device;
	std::shared_ptr<VulkanCore> core;

	/*  */
	VkQueue queue; // TODO rename graphicsQueue
	VkQueue presentQueue;

	/*  */
	uint32_t graphics_queue_node_index;
	VkCommandPool cmd_pool;
	VkCommandPool compute_pool;
	VkCommandPool transfer_pool;
};
class TmpVKSampleWindow : public VKWindow, public VKSampleSession {
  public:
};
/**
 * @brief
 *
 */
// TODO rename
template <class T> class VKSampleWindow {
  public:
	VKSampleWindow(int argc, const char **argv, std::unordered_map<const char *, bool> required_device_extensions = {},
				   std::unordered_map<const char *, bool> required_instance_layers = {},
				   std::unordered_map<const char *, bool> required_instance_extensions = {}) {

		/*	Parse argument.	*/
		const std::string helperInfo = "Vulkan Sample\n"
									   "A simple program for checking error detection"
									   "";
		/*	*/
		cxxopts::Options options("Vulkan Sample", helperInfo);
		options.add_options()("v,version", "Version information")("h,help", "helper information.")(
			"d,debug", "Enable Debug View.", cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
			"i,instance-extensions", "Size of each messages in bytes.", cxxopts::value<uint32_t>()->default_value("5"))(
			"l,instance-layers", "Size of each messages in bytes.", cxxopts::value<uint32_t>()->default_value("5"))(
			"D,device-extensions", "Perform Error Correction.", cxxopts::value<bool>()->default_value("false"))(
			"g,gpu-device", "GPU Device Select", cxxopts::value<int32_t>()->default_value("-1"))(
			"p,present-mode", "Present Mode", cxxopts::value<int32_t>()->default_value("-1"));

		auto result = options.parse(argc, (char **&)argv);
		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help();
			exit(EXIT_SUCCESS);
			// TODO exit
		}
		// if (result.count("version") > 0) {
		// 	std::cout << "Version: " << CRC_ANALYSIS_STR << " hash: " << CRC_ANALYSIS_GITCOMMIT_STR
		// 			  << " branch: " << CRC_ANALYSIS_GITBRANCH_TR << std::endl;
		// 	return EXIT_SUCCESS;
		// }
		bool debug = result["debug"].as<bool>();
		if (result.count("time") > 0) {
		}
		if (result.count("gpu-device") > 0) {
		}
		result["gpu-device"].count();
		int device_index = result["gpu-device"].as<int32_t>();

		std::unordered_map<const char *, bool> use_required_device_extensions = {};
		std::unordered_map<const char *, bool> use_required_instance_layers = {{"VK_LAYER_KHRONOS_validation", debug}};
		std::unordered_map<const char *, bool> use_required_instance_extensions = {
			{VK_KHR_SURFACE_EXTENSION_NAME, true}, {"VK_KHR_xlib_surface", true}};

		// TODO append to device extension.
		use_required_device_extensions.merge(required_device_extensions);
		use_required_instance_layers.merge(required_instance_layers);
		use_required_instance_extensions.merge(required_instance_extensions);

		std::vector<const char *> required_extension = VKWindow::getRequiredDeviceExtensions();
		for (auto it = required_extension.cbegin(); it != required_extension.cend(); it++) {
			required_device_extensions[(*it)] = true;
		}

		/*	Vulkan core.	*/
		this->core = std::make_shared<VulkanCore>(required_instance_extensions, required_instance_layers);

		/*	All physical devices.	*/
		std::vector<std::shared_ptr<PhysicalDevice>> physical_devices;
		bool group_device_request = device_index > 1;
		if (device_index >= 0) {
			// TODO validate if valid device index.
			if (device_index > core->getNrPhysicalDevices())
				throw cxxexcept::RuntimeException("Must be valid physical device {} greater than {}", device_index,
												  core->getNrPhysicalDevices());
			physical_devices.push_back(core->createPhysicalDevice(device_index));
		} else {
			physical_devices = core->createPhysicalDevices();
		}

		// TODO print all selected devices!
		std::cout << physical_devices[0]->getDeviceName() << std::endl;
		this->ldevice = std::make_shared<VKDevice>(physical_devices);

		this->ref = new T(core, ldevice);
	}

	void run() { this->ref->run(); }

	void screenshot(float scale) {}

	~VKSampleWindow() { delete ref; }

  private:
	T *ref;
	std::shared_ptr<VulkanCore> core;
	std::shared_ptr<VKDevice> ldevice;
};

#endif
