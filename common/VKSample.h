/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Exception.hpp"
#include "TaskScheduler/IScheduler.h"
#include "Util/PipelineLayoutUtil.h"
#include "VKSampleSession.h"
#include "VkPhysicalDevice.h"
#include "vulkan/vulkan_core.h"
#include <Core/SystemInfo.h>
#include <GeometryUtil.h>
#include <ProceduralGeometry.h>
#include <SDLDisplay.h>
#include <TaskScheduler.h>
#include <Util/IOUtil.h>
#include <VKWindow.h>
#include <cxxopts.hpp>
#include <memory>
#include <thread>

/**
 * @brief
 *
 * @tparam T
 */
template <class T> class VKSample : public vksample::VKSampleSession {
  public:
	VKSample() = default;

	void run(int argc, const char **argv, std::unordered_map<const char *, bool> required_device_extensions = {},
			 std::unordered_map<const char *, bool> required_instance_layers = {},
			 std::unordered_map<const char *, bool> required_instance_extensions = {}) override {

		/*	Parse argument.	*/
		const std::string helperInfo = "Vulkan Sample: " + fragcore::SystemInfo::getApplicationName() +
									   "\n"
									   "";
		/*	*/
		cxxopts::Options options("Vulkan Sample: " + fragcore::SystemInfo::getApplicationName(), helperInfo);
		cxxopts::OptionAdder &addr = options.add_options()("h,help", "helper information.")(
			"d,debug", "Enable Debug.", cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
			"i,instance-extensions", ".", cxxopts::value<uint32_t>()->default_value("5"))(
			"l,instance-layers", ".", cxxopts::value<uint32_t>()->default_value("5"))(
			"E,device-extensions", ".", cxxopts::value<bool>()->default_value("false"))(
			"g,gpu-device", "GPU Device Select", cxxopts::value<int32_t>()->default_value("-1"))(
			"p,present-mode", "Present Mode ()", cxxopts::value<int32_t>()->default_value("-1"))(
			"f,fullscreen", "FullScreen", cxxopts::value<bool>()->default_value("false"))(
			"a,headless", "Headless Renderer", cxxopts::value<bool>()->default_value("false"))(
			"r,renderdoc", "Enable RenderDoc ()", cxxopts::value<bool>()->default_value("false"))(
			"F,filesystem", "Set FileSystem, either directory or archive file (zip)",
			cxxopts::value<std::string>()->default_value("."))("C,color-space",
															   "Set the Display ColorSpace (Linear,SRGB)",
															   cxxopts::value<std::string>()->default_value(""))(
			"W,width", "Set Window Width in Pixels", cxxopts::value<int>()->default_value("-1"))(
			"H,height", "Set Window Height in Pixels", cxxopts::value<int>()->default_value("-1"))(
			"D,display", "Set Display index where the window will show", cxxopts::value<int>()->default_value("-1"))(
			"m,multi-sample", "Set MSAA (Multisampling Anti Aliasing) (2,4,8)",
			cxxopts::value<int>()->default_value("0"))("R,dynamic-range", "Set Dynamic Range ldr,hdr16,hdr32",
													   cxxopts::value<std::string>()->default_value("hdr16"))(
			"P,use-postprocessing", "Use Post Processing", cxxopts::value<bool>()->default_value("true"));
		// TODO: compute queue present

		/*	Append command option for the specific sample.	*/
		this->customOptions(addr);

		/*	Parse the command line input.	*/
		options.allow_unrecognised_options();
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

		const bool headless = result["headless"].as<bool>();

		/*	*/
		// TODO add support to integrate
		int nr_instance_extensions = result["instance-extensions"].count();
		int nr_instance_layers = result["instance-layers"].count();
		int nr_device_extensions = result["device-extensions"].count();
		int32_t device_select_index = result["gpu-device"].as<int32_t>();

		fragcore::Ref<fragcore::IScheduler> schedular =
			fragcore::Ref<fragcore::IScheduler>(new fragcore::TaskScheduler(2));

		/*	Create filesystem that the asset will be read from.	*/
		this->activeFileSystem = fragcore::FileSystem::createFileSystem(schedular);
		const std::string filesystemPath = result["filesystem"].as<std::string>();
		if (!this->activeFileSystem->isDirectory(filesystemPath.c_str())) {

			const std::string extension = this->activeFileSystem->getFileExtension(filesystemPath.c_str());
			if (extension == ".zip") {
				std::cout << "Found Zip File System: " << filesystemPath << std::endl;
				this->activeFileSystem = fragcore::ZipFileSystem::createZipFileObject(filesystemPath.c_str());
			}
		}

		// TODO add surface extension based on platform.
		std::unordered_map<const char *, bool> use_required_device_extensions = {
			{VK_KHR_SWAPCHAIN_EXTENSION_NAME, !headless}, {VK_KHR_MAINTENANCE1_EXTENSION_NAME, true}};
		std::unordered_map<const char *, bool> use_required_instance_layers = {{"VK_LAYER_KHRONOS_validation", debug}};
		std::unordered_map<const char *, bool> use_required_instance_extensions = {
			{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, debug},
			{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, debug},
			{VK_KHR_DISPLAY_EXTENSION_NAME, !headless},
			{"VK_KHR_surface", !headless},
			{VK_KHR_SURFACE_EXTENSION_NAME, !headless},
			{"VK_KHR_xlib_surface", !headless},
			{"VK_KHR_display", !headless}};

		// TODO append to device extension.
		use_required_device_extensions.merge(required_device_extensions);
		use_required_instance_layers.merge(required_instance_layers);
		use_required_instance_extensions.merge(required_instance_extensions);

		/*	*/
		std::vector<const char *> required_window_device_extensions =
			vksample::VKBaseSampleWindow::getRequiredDeviceExtensions();
		// TODO: add window required if using window
		for (auto it = required_window_device_extensions.cbegin(); it != required_window_device_extensions.cend();
			 it++) {
			required_device_extensions[(*it)] = true;
		}

		/*	Vulkan core.	*/
		this->core =
			std::make_shared<fvkcore::VulkanCore>(use_required_instance_extensions, use_required_instance_layers);

		if (device_select_index <= -1) {

			const std::vector<std::shared_ptr<fvkcore::PhysicalDevice>> physical_devices =
				this->core->createPhysicalDevices();

			if (physical_devices.size() == 0) {
				throw cxxexcept::RuntimeException("Could not retrive any physical devies");
			}
			device_select_index = 0;
			for (size_t index_phy = 0; index_phy < physical_devices.size(); index_phy++) {
				VkPhysicalDeviceType deviceType = physical_devices[index_phy]->getProperties().deviceType;
				if (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					device_select_index = (int)index_phy;
					break;
				}
			}
		}

		/*	All physical devices.	*/
		std::vector<std::shared_ptr<fvkcore::PhysicalDevice>> selected_physical_devices;
		const bool group_device_request = result.count("gpu-device") > 1;

		if (group_device_request) {
			required_device_extensions[VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME] = true;
		}

		if (device_select_index >= 0 && device_select_index < this->core->getNrPhysicalDevices()) {
			selected_physical_devices.push_back(core->createPhysicalDevice(device_select_index));
		} else {
			throw cxxexcept::RuntimeException("Failed to find physical device");
		}

		/*	Check if device extensions are supported.	*/
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

		/*	Select All Queue and Create Device.	*/
		{
			std::vector<std::vector<float>> global_queuePriorities;
			std::vector<VkDeviceQueueCreateInfo> queues = this->OnSelectQueue(selected_physical_devices);

			/*	Select */
			if (queues.size() == 0) {
				for (size_t j = 0; j < selected_physical_devices[0]->getQueueFamilyProperties().size(); j++) {

					/*  */
					const VkQueueFamilyProperties &familyProp =
						selected_physical_devices[0]->getQueueFamilyProperties()[j];
					std::vector<float> queuePriorities(familyProp.queueCount, 1.0f);
					global_queuePriorities.push_back(queuePriorities);

					/*	*/
					VkDeviceQueueCreateInfo queueCreateInfo;
					queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueCreateInfo.pNext = nullptr;
					queueCreateInfo.flags = 0;
					queueCreateInfo.queueFamilyIndex = j;
					queueCreateInfo.queueCount = familyProp.queueCount;
					queueCreateInfo.pQueuePriorities = global_queuePriorities.back().data();

					queues.push_back(queueCreateInfo);
				}
			}else{
				throw cxxexcept::RuntimeException("No Queue Selected");
			}

			this->ldevice =
				std::make_shared<fvkcore::VKDevice>(selected_physical_devices, use_required_device_extensions, queues);
		}

		/*	Create Sample Object.	*/
		this->ref = new T(core, ldevice);
		/*	Pass custom command options.	*/
		this->ref->setCommandResult(result);

		/*	Internal initialize.	*/
		this->ref->setFileSystem(this->activeFileSystem);

		/*	Only if sample is a window type.	*/
		fragcore::Window *windowRef = dynamic_cast<fragcore::Window *>(this->ref);
		if constexpr (std::is_base_of_v<T, vksample::VKBaseSampleWindow> && windowRef && !headless) {
			int width = result["width"].as<int>();
			int height = result["height"].as<int>();
			const int display_index = result["display"].as<int>();
			int window_x = 0, window_y = 0;

			fragcore::SDLDisplay display = fragcore::SDLDisplay::getPrimaryDisplay();
			if (display_index >= 0) {
				display = fragcore::SDLDisplay::getDisplay(display_index);
			}

			/*	*/
			if (fullscreen) {
				/* Compute window size	*/
				width = display.width();
				height = display.height();

				window_x = display.x();
				window_y = display.y();
			} else if (width == -1 || height == -1) {

				/* Compute window size	*/
				width = display.width() / 2;
				height = display.height() / 2;

				window_x = display.x() + width;
				window_y = display.y() + height;
			}
			windowRef->setPosition(window_x, window_y);
			windowRef->setSize(width, height);

			// windowRef->vsync(vsync);
			windowRef->setFullScreen(fullscreen);
			windowRef->show();
		}

		/*	*/
		this->ref->debug(debug);

		/*	*/
		this->ref->run();
	}

	virtual std::vector<VkDeviceQueueCreateInfo> OnSelectQueue(
		[[maybe_unused]] const std::vector<std::shared_ptr<fvkcore::PhysicalDevice>> &physical_selected_devices) {

		std::vector<std::vector<float>> global_queuePriorities;
		std::vector<VkDeviceQueueCreateInfo> queues;

		for (size_t queue_family_index = 0;
			 queue_family_index < physical_selected_devices[0]->getQueueFamilyProperties().size();
			 queue_family_index++) {
			/*  */
			const VkQueueFamilyProperties &familyProp =
				physical_selected_devices[0]->getQueueFamilyProperties()[queue_family_index];
			std::vector<float> queuePriorities(familyProp.queueCount, 1.0f);
			global_queuePriorities.push_back(queuePriorities);

			VkDeviceQueueCreateInfo queueCreateInfo;
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pNext = nullptr;
			queueCreateInfo.flags = 0;
			queueCreateInfo.queueFamilyIndex = queue_family_index;
			queueCreateInfo.queueCount = familyProp.queueCount;
			queueCreateInfo.pQueuePriorities = global_queuePriorities.back().data();

			queues.push_back(queueCreateInfo);
		}

		return {};
	}

	~VKSample() override {
		// this->ref->Release();
		delete this->ref;
	}

  private:
	T *ref;
	std::shared_ptr<fvkcore::VulkanCore> core;
	std::shared_ptr<fvkcore::VKDevice> ldevice;
};
