#ifndef _VK_SAMPLE_WINDOW_H_
#define _VK_SAMPLE_WINDOW_H_ 1
#include "FPSCounter.h"
#include "Util/Time.hpp"
#include "VKWindow.h"
#include <cxxopts.hpp>
#include <iostream>
#include <map>

using namespace fvkcore;

class VKSampleSession {
  public:
	VKSampleSession(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: core(core), device(device) {}
	virtual void run() {}
	virtual void release() {}
	virtual void commandline(cxxopts::Options &options) {}

	virtual ~VKSampleSession() {}

  public:
	FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }

  public:
	/*	*/
	VkDevice getDevice() const noexcept { return this->device->getHandle(); }

	uint32_t getGraphicQueueIndex() const;
	VkQueue getDefaultGraphicQueue() const;
	VkQueue getDefaultComputeQueue() const;

	const std::shared_ptr<VKDevice> &getVKDevice() const noexcept { return this->device; }
	const std::shared_ptr<PhysicalDevice> getPhysicalDevice() const noexcept {
		return this->getVKDevice()->getPhysicalDevice(0);
	}
	std::shared_ptr<PhysicalDevice> getPhysicalDevice() noexcept { return this->getVKDevice()->getPhysicalDevice(0); }

	VkPhysicalDevice physicalDevice() const { return this->getPhysicalDevice()->getHandle(); }
	void setPhysicalDevice(VkPhysicalDevice device);
	std::vector<VkQueue> getQueues() const noexcept { return {}; }
	const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const { return core->getPhysicalDevices(); }

	/*	*/
	VkInstance getInstance() const noexcept { return this->core->getHandle(); }

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

  protected:
	FPSCounter<float> fpsCounter;
	vkscommon::Time time;
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
									   ""
									   "";
		/*	*/
		cxxopts::Options options("Vulkan Sample", helperInfo);
		options.add_options()("h,help", "helper information.")("d,debug", "Enable Debug View.",
															   cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
			"i,instance-extensions", "Size of each messages in bytes.", cxxopts::value<uint32_t>()->default_value("5"))(
			"l,instance-layers", "Size of each messages in bytes.", cxxopts::value<uint32_t>()->default_value("5"))(
			"D,device-extensions", "Perform Error Correction.", cxxopts::value<bool>()->default_value("false"))(
			"g,gpu-device", "GPU Device Select", cxxopts::value<int32_t>()->default_value("-1"))(
			"p,present-mode", "Present Mode", cxxopts::value<int32_t>()->default_value("-1"))(
			"f,fullscreen", "FullScreen", cxxopts::value<bool>()->default_value("false"))(
			"H,headless", "Headless Renderer", cxxopts::value<bool>()->default_value("false"))(
			"r,renderdoc", "Enable RenderDoc", cxxopts::value<bool>()->default_value("false"));

		auto result = options.parse(argc, (char **&)argv);
		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help();
			exit(EXIT_SUCCESS);
		}

		/*	*/
		bool debug = result["debug"].as<bool>();
		bool fullscreen = result["fullscreen"].as<bool>();
		if (result.count("time") > 0) {
		}
		if (result.count("gpu-device") > 0) {
		}
		bool headless = result["headless"].as<bool>();

		/*	*/
		int nr_instance_extensions = result["instance-extensions"].count();
		int nr_instance_layers = result["instance-layers"].count();
		int nr_device_extensions = result["device-extensions"].count();
		int device_index = result["gpu-device"].as<int32_t>();

		// TODO add surface extension based on platform.
		std::unordered_map<const char *, bool> use_required_device_extensions = {
			{VK_KHR_SWAPCHAIN_EXTENSION_NAME, !headless}};
		std::unordered_map<const char *, bool> use_required_instance_layers = {{"VK_LAYER_KHRONOS_validation", debug}};
		std::unordered_map<const char *, bool> use_required_instance_extensions = {
			{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, debug},
			{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, debug},
			{VK_KHR_DISPLAY_EXTENSION_NAME, !headless},
			{"VK_KHR_surface", !headless},
			{VK_KHR_SURFACE_EXTENSION_NAME, !headless},
			{"VK_KHR_xlib_surface", !headless},
			{"VK_KHR_display", !headless}};
		//

		// TODO append to device extension.
		use_required_device_extensions.merge(required_device_extensions);
		use_required_instance_layers.merge(required_instance_layers);
		use_required_instance_extensions.merge(required_instance_extensions);

		/*	*/
		std::vector<const char *> required_extension = VKWindow::getRequiredDeviceExtensions();
		for (auto it = required_extension.cbegin(); it != required_extension.cend(); it++) {
			required_device_extensions[(*it)] = true;
		}

		/*	Vulkan core.	*/
		this->core = std::make_shared<VulkanCore>(use_required_instance_extensions, use_required_instance_layers);

		/*	All physical devices.	*/
		std::vector<std::shared_ptr<PhysicalDevice>> physical_devices;
		bool group_device_request = device_index > 1;
		if (device_index >= 0) {
			// TODO validate if valid device index.
			if ((uint32_t)device_index > core->getNrPhysicalDevices())
				throw cxxexcept::RuntimeException("Must be valid physical device {} greater than {}", device_index,
												  core->getNrPhysicalDevices());
			physical_devices.push_back(core->createPhysicalDevice(device_index));
		} else {
			physical_devices = core->createPhysicalDevices();
		}

		/*	*/
		for (size_t i = 0; i < physical_devices.size(); i++) {
			std::cout << physical_devices[i]->getDeviceName() << std::endl;
		}

		this->ldevice = std::make_shared<VKDevice>(physical_devices, use_required_device_extensions);

		int width, height;
		if (fullscreen) {
			// Display
		}

		/*	*/
		this->ref = new T(core, ldevice);
	}

	void run() { this->ref->run(); }

	void screenshot(float scale) {}

	virtual ~VKSampleWindow() { delete ref; }

  private:
	T *ref;
	std::shared_ptr<VulkanCore> core;
	std::shared_ptr<VKDevice> ldevice;
};

#endif
