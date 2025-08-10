/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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
#include "IO/IFileSystem.h"
#include "spdlog/logger.h"
#include "vulkan/vulkan_core.h"
#include <Core/Object.h>
#include <Core/Time.h>
#include <VKDevice.h>
#include <VkPhysicalDevice.h>
#include <VulkanCore.h>
#include <cxxopts.hpp>
#include <fmt/format.h>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC VKSampleSessionBase : fragcore::Object {
	  public:
		VKSampleSessionBase(std::shared_ptr<fvkcore::VulkanCore> &core, std::shared_ptr<fvkcore::VKDevice> &device);
		~VKSampleSessionBase() override;

		virtual void run() = 0;
		virtual void Initialize() {}
		virtual void release() {}

	  public: /*	*/
		virtual void loadDefaultQueue();
		virtual void loadDescriptorPool();
		virtual void loadCachePipeline();

	  public: /*	*/
		/*	*/
		fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }
		void setFileSystem(fragcore::IFileSystem *filesystem) { this->filesystem = filesystem; }

		/*	*/
		cxxopts::ParseResult &getResult() noexcept { return this->parseResult; }
		void setCommandResult(cxxopts::ParseResult &result) noexcept { this->parseResult = result; }

		/*	*/
		const fragcore::Time &getTimer() const noexcept { return this->time; }
		fragcore::Time &getTimer() noexcept { return this->time; }

		spdlog::logger &getLogger() const noexcept { return *this->logger; }

		void debug(const bool enable);

	  public: /*	Helper Methods.	*/
		void allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, VkBuffer &buffer,
							VkDeviceMemory &bufferMemory, void *pNext = nullptr);
		void allocateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
						   VkImageUsageFlags usage, VkMemoryPropertyFlags properties, const VkImageCreateFlags flags,
						   VkImage &image, VkDeviceMemory &imageMemory, void *pNext = nullptr);
		// TODO: array of data.pointer
		void transferBufferData(VkBuffer buffer, VkDeviceMemory memory, const void *pData, const size_t sizeInBytes,
								const size_t offset = 0);
		void transferImageData(VkImage image, VkDeviceMemory imageMemory, const size_t width, const size_t height,
							   const size_t depth, const void *pData, const size_t sizeInBytes,
							   const size_t offset = 0);

		VkCommandBuffer getTransferCommandBuffer() const noexcept;
		void endTransferCommand(VkCommandBuffer cmd) const noexcept;

	  public: /*	Vulkan methods.	*/
		/*	*/
		VkInstance getInstance() const noexcept { return this->core->getHandle(); }

		VkDevice getDevice() const noexcept { return this->device->getHandle(); }

		uint32_t getDefaultGraphicQueueIndex() const noexcept { return this->graphics_queue_node_index; }
		uint32_t getDefaultComputeQueueIndex() const noexcept { return this->compute_queue_node_index; }
		uint32_t getDefaultTransferQueueIndex() const noexcept { return this->transfer_queue_node_index; }

		VkQueue getDefaultGraphicQueue() const noexcept { return this->graphic_queue; }
		VkQueue getDefaultComputeQueue() const noexcept { return this->compute_queue; }
		VkQueue getDefaultTransferQueue() const noexcept { return this->transfer_queue; }

		const std::shared_ptr<fvkcore::VKDevice> &getVKDevice() const noexcept { return this->device; }
		std::shared_ptr<fvkcore::VKDevice> &getVKDevice() noexcept { return this->device; }

		std::shared_ptr<fvkcore::PhysicalDevice> getPhysicalDevice() const noexcept {
			return this->getVKDevice()->getPhysicalDevice(0);
		}
		std::shared_ptr<fvkcore::PhysicalDevice> getPhysicalDevice() noexcept {
			return this->getVKDevice()->getPhysicalDevice(0);
		}

		const VkPhysicalDeviceProperties &physicalDeviceProperties() const noexcept;

		VkPhysicalDevice physicalDevice() const { return this->getPhysicalDevice()->getHandle(); }
		void setPhysicalDevice(VkPhysicalDevice device);

		std::vector<VkQueue> getQueues() const noexcept { return {}; }
		const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const {
			return this->core->getPhysicalDevices();
		}

		VkCommandPool getGraphicCommandPool() const noexcept { return this->graphic_pool; }
		VkCommandPool getTransferCommandPool() const noexcept { return this->transfer_pool; }
		VkCommandPool getComputeommandPool() const noexcept { return this->compute_pool; }

	  public:
		VkDescriptorPool getDescriptorPool() const noexcept { return this->desc_pool; }
		const VkAllocationCallbacks *getAllocatorCallback() const noexcept { return this->g_Allocator; }
		VkPipelineCache getPipelineCache() const noexcept { return this->pipelineCache; }

	  protected: /*	*/
		std::shared_ptr<fvkcore::VulkanCore> core;
		std::shared_ptr<fvkcore::VKDevice> device;

		/*  */
		VkQueue graphic_queue{};
		VkQueue compute_queue{};
		VkQueue transfer_queue{};

		/*  */
		uint32_t graphics_queue_node_index{};
		uint32_t compute_queue_node_index{};
		uint32_t transfer_queue_node_index{};

		VkCommandPool graphic_pool{};
		VkCommandPool compute_pool{};
		VkCommandPool transfer_pool{};

		VkDescriptorPool desc_pool;

		VkAllocationCallbacks *g_Allocator = nullptr;
		VkPipelineCache pipelineCache = VK_NULL_HANDLE;

		VkQueryPool queryPool;

		/*	Features.	*/
		bool dynamic_rendering{false};
		bool hasDynamicState{false};
		bool hasDynamicState2{false};
		bool hasDynamicState3{false};
		bool useFragmentShadingRate{false};
		bool useHostImageCopy{false};
		bool useDescriptorIndex{false};

		/*	Properties.	*/

	  private: /*	*/
		cxxopts::ParseResult parseResult;
		fragcore::Time time;
		spdlog::logger *logger = nullptr;
		fragcore::IFileSystem *filesystem = nullptr;
		void *rdoc_api = nullptr;
	};

} // namespace vksample
