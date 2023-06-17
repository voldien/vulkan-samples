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
			: core(core), device(device) {
			this->loadDefaultQueue();
		}

		virtual void run() = 0;
		virtual void release() {}

		virtual void loadDefaultQueue() {
			/*	*/
			this->graphics_queue_node_index = 0;
			this->compute_queue_node_index = 0;
			this->transfer_queue_node_index = 0;

			/*	*/
			this->graphic_queue = this->device->getQueue(this->graphics_queue_node_index, 0);
			this->compute_queue = this->device->getQueue(this->compute_queue_node_index, 0);
			this->transfer_queue = this->device->getQueue(this->transfer_queue_node_index, 0);
		}

		virtual ~VKSampleSessionBase() { this->release(); }

	  public: /*	*/
		FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }

		cxxopts::ParseResult &getResult() { return this->parseResult; }
		void setCommandResult(cxxopts::ParseResult &result) { this->parseResult = result; }

		fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }

		void setFileSystem(fragcore::IFileSystem *filesystem) { this->filesystem = filesystem; }

	  public: /*	Vulkan methods.	*/
		VkDevice getDevice() const noexcept { return this->device->getHandle(); }

		uint32_t getDefaultGraphicQueueIndex() const { return this->graphics_queue_node_index; }
		uint32_t getDefaultComputeQueueIndex() const { return this->compute_queue_node_index; }
		uint32_t getDefaultTransferQueueIndex() const { return this->transfer_queue_node_index; }
		VkQueue getDefaultGraphicQueue() const { return this->graphic_queue; }
		VkQueue getDefaultComputeQueue() const { return this->compute_queue; }
		VkQueue getDefaultTransferQueue() const { return this->transfer_queue; }

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
		const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const {
			return this->core->getPhysicalDevices();
		}

		/*	*/
		VkInstance getInstance() const noexcept { return this->core->getHandle(); }

		VkCommandPool getGraphicCommandPool() const noexcept { return this->graphic_pool; }
		VkCommandPool getTransferCommandPool() const noexcept { return this->transfer_pool; }
		VkCommandPool getComputeommandPool() const noexcept { return this->compute_pool; }

	  protected: /*	*/
		std::shared_ptr<fvkcore::VulkanCore> core;
		std::shared_ptr<fvkcore::VKDevice> device;

		/*  */
		VkQueue graphic_queue;
		VkQueue compute_queue;
		VkQueue transfer_queue;

		/*  */
		uint32_t graphics_queue_node_index;
		uint32_t compute_queue_node_index;
		uint32_t transfer_queue_node_index;

		VkCommandPool graphic_pool;
		VkCommandPool compute_pool;
		VkCommandPool transfer_pool;

	  protected: /*	*/
		cxxopts::ParseResult parseResult;
		FPSCounter<float> fpsCounter;
		vkscommon::Time time;
		fragcore::IFileSystem *filesystem;
	};
} // namespace vkscommon
