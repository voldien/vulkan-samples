#pragma once
#include "Importer/IOUtil.h"
#include "Util/CameraController.h"
#include "VKSampleSession.h"
#include "VkPhysicalDevice.h"
#include "vulkan/vulkan_core.h"
#include <Core/SystemInfo.h>
#include <GeometryUtil.h>
#include <ProceduralGeometry.h>
#include <SDLDisplay.h>
#include <cxxopts.hpp>
#include <memory>

template <class T> class VKSample : public vkscommon::VKSampleSession {
  public:
	VKSample() {}

	virtual void run(int argc, const char **argv,
					 std::unordered_map<const char *, bool> required_device_extensions = {},
					 std::unordered_map<const char *, bool> required_instance_layers = {},
					 std::unordered_map<const char *, bool> required_instance_extensions = {}) override {

		/*	Parse argument.	*/
		const std::string helperInfo = "Vulkan Sample: " + fragcore::SystemInfo::getApplicationName() +
									   "\n"
									   "";
		/*	*/
		cxxopts::Options options("Vulkan Sample: " + fragcore::SystemInfo::getApplicationName(), helperInfo);
		cxxopts::OptionAdder &addr = options.add_options()("h,help", "helper information.")(
			"d,debug", "Enable Debug View.", cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
			"i,instance-extensions", ".", cxxopts::value<uint32_t>()->default_value("5"))(
			"l,instance-layers", ".", cxxopts::value<uint32_t>()->default_value("5"))(
			"D,device-extensions", ".", cxxopts::value<bool>()->default_value("false"))(
			"g,gpu-device", "GPU Device Select", cxxopts::value<int32_t>()->default_value("-1"))(
			"p,present-mode", "Present Mode", cxxopts::value<int32_t>()->default_value("-1"))(
			"f,fullscreen", "FullScreen", cxxopts::value<bool>()->default_value("false"))(
			"H,headless", "Headless Renderer", cxxopts::value<bool>()->default_value("false"))(
			"r,renderdoc", "Enable RenderDoc", cxxopts::value<bool>()->default_value("false"))(
			"F,filesystem", "FileSystem", cxxopts::value<std::string>()->default_value("."))(
			"C,color-space", "Display ColorSpace", cxxopts::value<std::string>()->default_value(""));

		/*	Append command option for the specific sample.	*/
		this->customOptions(addr);

		/*	Parse the command line input.	*/
		auto result = options.parse(argc, (char **&)argv);

		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help(options.groups()) << std::endl;
			exit(EXIT_SUCCESS);
		}

		/*	*/
		bool debug = result["debug"].as<bool>();
		bool fullscreen = result["fullscreen"].as<bool>();
		if (result.count("time") > 0) {
			/*	Create seperate thread that count down.*/
			if (result["time"].as<float>() > 0) {
				int64_t timeout_mili = (int64_t)(result["time"].as<float>() * 1000.0f);
				std::thread timeout_thread = std::thread([&]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(timeout_mili));
					exit(EXIT_SUCCESS);
				});
				timeout_thread.detach();
			}
		}

		if (result.count("gpu-device") > 0) {
		}
		bool headless = result["headless"].as<bool>();

		/*	*/
		// TODO add support to integrate
		int nr_instance_extensions = result["instance-extensions"].count();
		int nr_instance_layers = result["instance-layers"].count();
		int nr_device_extensions = result["device-extensions"].count();
		int device_index = result["gpu-device"].as<int32_t>();

		/*	Create filesystem that the asset will be read from.	*/
		this->activeFileSystem = fragcore::FileSystem::createFileSystem();
		std::string filesystemPath = result["filesystem"].as<std::string>();
		if (!this->activeFileSystem->isDirectory(filesystemPath.c_str())) {

			const std::string extension = this->activeFileSystem->getFileExtension(filesystemPath.c_str());
			if (extension == ".zip") {
				std::cout << "Found Zip File System: " << filesystemPath << std::endl;
				this->activeFileSystem = fragcore::ZipFileSystem::createZipFileObject(filesystemPath.c_str());
			}
		}

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
		std::vector<const char *> required_window_device_extensions = VKWindow::getRequiredDeviceExtensions();
		for (auto it = required_window_device_extensions.cbegin(); it != required_window_device_extensions.cend();
			 it++) {
			required_device_extensions[(*it)] = true;
		}

		/*	Vulkan core.	*/
		this->core = std::make_shared<VulkanCore>(use_required_instance_extensions, use_required_instance_layers);

		if (device_index >= this->core->getNrPhysicalDevices()) {
			throw cxxexcept::RuntimeException("Must be valid physical device {} greater than {}", device_index,
											  core->getNrPhysicalDevices());
		}

		/*	All physical devices.	*/
		std::vector<std::shared_ptr<PhysicalDevice>> selected_physical_devices;
		bool group_device_request = result.count("gpu-device") > 1;

		if (device_index >= 0) {
			selected_physical_devices.push_back(core->createPhysicalDevice(device_index));
		} else {
			// physical_devices.push_back(std::shared_ptr<PhysicalDevice>(core->getPhysicalDevices()[0]));
		}

		for (size_t i = 0; i < selected_physical_devices.size(); i++) {
			for (auto it = use_required_device_extensions.cbegin(); it != use_required_device_extensions.cend(); it++) {
				if (!selected_physical_devices[i]->isExtensionSupported((*it).first)) {
					throw cxxexcept::RuntimeException("Device: {} Does not support {}",
													  selected_physical_devices[i]->getDeviceName(), (*it).first);
				}
			}
		}

		/*	*/
		for (size_t i = 0; i < selected_physical_devices.size(); i++) {
			std::cout << selected_physical_devices[i]->getDeviceName() << std::endl;
		}

		/*Select All Queue and Create Device.	*/
		{
			std::vector<std::vector<float>> global_queuePriorities;
			std::vector<VkDeviceQueueCreateInfo> queues = this->OnSelectQueue(selected_physical_devices);

			if (queues.size() == 0) {
				for (size_t j = 0; j < selected_physical_devices[0]->getQueueFamilyProperties().size(); j++) {
					/*  */
					const VkQueueFamilyProperties &familyProp =
						selected_physical_devices[0]->getQueueFamilyProperties()[j];
					std::vector<float> queuePriorities(familyProp.queueCount, 1.0f);
					global_queuePriorities.push_back(queuePriorities);

					VkDeviceQueueCreateInfo queueCreateInfo;
					queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueCreateInfo.pNext = nullptr;
					queueCreateInfo.flags = 0;
					queueCreateInfo.queueFamilyIndex = j;
					queueCreateInfo.queueCount = familyProp.queueCount;
					queueCreateInfo.pQueuePriorities = global_queuePriorities.back().data();

					queues.push_back(queueCreateInfo);
				}
			}

			this->ldevice =
				std::make_shared<VKDevice>(selected_physical_devices, use_required_device_extensions, queues);
		}

		/*	*/
		{
			/*	Create Sample Object.	*/
			this->ref = new T(core, ldevice);
			/*	Pass custom command options.	*/
			this->ref->setCommandResult(result);

			/*	Internal initialize.	*/
			this->ref->setFileSystem(this->activeFileSystem);

			int width = -1;
			int height = -1;
			if (fullscreen) {
				// Display,
				fragcore::SDLDisplay display = fragcore::SDLDisplay::getPrimaryDisplay();
				width = display.width();
				height = display.height();
			}

			/*	Only if sample is a window type.	*/
			IWindow *windowRef = dynamic_cast<IWindow *>(this->ref);
			if (windowRef) {
				windowRef->setSize(width, height);
				// this->sampleRef->vsync(vsync);
				windowRef->setFullScreen(fullscreen);
			}

			this->ref->run();
		}
	}

	virtual std::vector<VkDeviceQueueCreateInfo> OnSelectQueue([
		[maybe_unused]] std::vector<std::shared_ptr<PhysicalDevice>> &physical_devices) {
		return {};
	}

	virtual ~VKSample() {
		// this->ref->Release();
		delete this->ref;
	}

  private:
	T *ref;
	std::shared_ptr<VulkanCore> core;
	std::shared_ptr<VKDevice> ldevice;
};
