#pragma once
#include "FPSCounter.h"
#include "Util/Time.hpp"
#include <Core/IO/FileSystem.h>
#include <VKDevice.h>
#include <VkPhysicalDevice.h>
#include <VulkanCore.h>
#include <cxxopts.hpp>
#include <iostream>
#include <map>

namespace vkscommon {

	class FVDECLSPEC VKSampleSessionBase {
	  public:
		VKSampleSessionBase(std::shared_ptr<fvkcore::VulkanCore> &core, std::shared_ptr<fvkcore::VKDevice> &device)
			: core(core), device(device) {}
		virtual void run() = 0;
		virtual void release() {}

		virtual ~VKSampleSessionBase() { this->release(); }

	  public: /*	*/
		FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }

		cxxopts::ParseResult &getResult() { return this->parseResult; }
		void setCommandResult(cxxopts::ParseResult &result) { this->parseResult = result; }

		fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }

		void setFileSystem(fragcore::IFileSystem *filesystem) { this->filesystem = filesystem; }

	  public: /*	Vulkan methods.	*/
		VkDevice getDevice() const noexcept { return this->device->getHandle(); }

		uint32_t getGraphicQueueIndex() const;
		VkQueue getDefaultGraphicQueue() const;
		VkQueue getDefaultComputeQueue() const;

		const std::shared_ptr<fvkcore::VKDevice> &getVKDevice() const noexcept { return this->device; }
		std::shared_ptr<fvkcore::VKDevice> &getVKDevice() noexcept { return this->device; }

		const std::shared_ptr<fvkcore::PhysicalDevice> getPhysicalDevice() const noexcept {
			return this->getVKDevice()->getPhysicalDevice(0);
		}
		std::shared_ptr<fvkcore::PhysicalDevice> getPhysicalDevice() noexcept {
			return this->getVKDevice()->getPhysicalDevice(0);
		}

		VkPhysicalDevice physicalDevice() const { return this->getPhysicalDevice()->getHandle(); }
		void setPhysicalDevice(VkPhysicalDevice device);
		std::vector<VkQueue> getQueues() const noexcept { return {}; }
		const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const { return core->getPhysicalDevices(); }

		/*	*/
		VkInstance getInstance() const noexcept { return this->core->getHandle(); }

	  protected: /*	*/
		std::shared_ptr<fvkcore::VulkanCore> core;
		std::shared_ptr<fvkcore::VKDevice> device;

		/*  */
		VkQueue queue; // TODO rename graphicsQueue
		VkQueue presentQueue;

		/*  */
		uint32_t graphics_queue_node_index;
		VkCommandPool cmd_pool;
		VkCommandPool compute_pool;
		VkCommandPool transfer_pool;

	  protected: /*	*/
		cxxopts::ParseResult parseResult;
		FPSCounter<float> fpsCounter;
		vkscommon::Time time;
		fragcore::IFileSystem *filesystem;
	};
} // namespace vkscommon
