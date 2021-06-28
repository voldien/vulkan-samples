#ifndef _VK_SAMPLES_COMMON_VK_DEVICE_H_
#define _VK_SAMPLES_COMMON_VK_DEVICE_H_ 1
#include "VKHelper.h"
#include "VKUtil.h"
#include "VkPhysicalDevice.h"
#include "VulkanCore.h"
#include <fmt/core.h>
#include <optional>
#include <unordered_map>

/**
 * @brief
 *
 */
class VKDevice {
  public:
	// TODO add support for group device.!
	/**
	 * @brief Construct a new VKDevice object
	 *
	 * @param physicalDevices
	 * @param requested_extensions
	 * @param requiredQueues
	 */
	VKDevice(const std::vector<std::shared_ptr<PhysicalDevice>> &physicalDevices,
			 const std::unordered_map<const char *, bool> &requested_extensions = {},
			 VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
	// TODO add std::fucntion for override the select GPU.

	VKDevice(const std::shared_ptr<PhysicalDevice> &physicalDevice,
			 const std::unordered_map<const char *, bool> &requested_extensions = {},
			 VkQueueFlags requiredQueues = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
	VKDevice(const VKDevice &) = delete;
	VKDevice(VKDevice &&) = delete;
	~VKDevice(void);

	/**
	 * @brief
	 *
	 * @return true
	 * @return false
	 */
	bool isGroupDevice(void) const noexcept { return getPhysicalDevices().size() > 0; }
	const std::vector<std::shared_ptr<PhysicalDevice>> &getPhysicalDevices(void) const noexcept {
		return physicalDevices;
	}

	const std::shared_ptr<PhysicalDevice> &getPhysicalDevice(unsigned int index) const {
		return physicalDevices[index];
	}

	VkDevice getHandle(void) const noexcept { return this->logicalDevice; }

	VkQueue getDefaultGraphicQueue(void) const noexcept { return this->graphicsQueue; }
	VkQueue getDefaultPresent(void) const noexcept { return this->presentQueue; }
	VkQueue getDefaultCompute(void) const noexcept { return this->computeQueue; }
	VkQueue getDefaultTransfer(void) const noexcept { return this->transferQueue; }

	uint32_t getDefaultGraphicQueueIndex(void) const noexcept { return this->graphics_queue_node_index; }
	uint32_t getDefaultComputeQueueIndex(void) const noexcept { return this->compute_queue_node_index; }
	uint32_t getDefaultTransferQueueIndex(void) const noexcept { return this->transfer_queue_node_index; }

	/**
	 * @brief
	 *
	 * @param typeFilter
	 * @param properties
	 * @return uint32_t
	 */
	template <size_t n = 0>
	std::optional<uint32_t> findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
		return VKHelper::findMemoryType(physicalDevices[0]->getMemoryProperties(), typeFilter, properties);
	}

	/**
	 * @brief Create a Command Pool object
	 *
	 * @param queue
	 * @param flag
	 * @param pNext
	 * @return VkCommandPool
	 */
	VkCommandPool createCommandPool(uint32_t queue,
									VkCommandPoolCreateFlags flag = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
									const void *pNext = nullptr) {
		VkCommandPool pool;
		/*  Create command pool.    */
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.pNext = pNext;
		cmdPoolCreateInfo.queueFamilyIndex = queue;
		cmdPoolCreateInfo.flags = flag;

		/*  Create command pool.    */
		VKS_VALIDATE(vkCreateCommandPool(getHandle(), &cmdPoolCreateInfo, nullptr, &pool));

		return pool;
	}

	void submitCommands(VkQueue queue, const std::vector<VkCommandBuffer> &cmd,
						const std::vector<VkSemaphore> &waitSemaphores = {},
						const std::vector<VkSemaphore> &signalSempores = {}, VkFence fence = VK_NULL_HANDLE,
						const std::vector<VkPipelineStageFlags> &waitStages = {
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}) {
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = waitSemaphores.size();
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();

		/*	*/
		submitInfo.commandBufferCount = cmd.size();
		submitInfo.pCommandBuffers = cmd.data();

		/*	*/
		submitInfo.signalSemaphoreCount = signalSempores.size();
		submitInfo.pSignalSemaphores = signalSempores.data();

		VKS_VALIDATE(vkQueueSubmit(queue, 1, &submitInfo, fence));
	}

	std::vector<VkCommandBuffer> allocateCommandBuffers(VkCommandPool commandPool, VkCommandBufferLevel level,
														unsigned int nrCmdBuffers = 1, const void *pNext = nullptr) {
		std::vector<VkCommandBuffer> cmdBuffers(nrCmdBuffers);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.pNext = pNext;
		allocInfo.level = level;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = nrCmdBuffers;

		VKS_VALIDATE(vkAllocateCommandBuffers(getHandle(), &allocInfo, cmdBuffers.data()));

		return cmdBuffers;
	}

	std::vector<VkCommandBuffer>
	beginSingleTimeCommands(VkCommandPool commandPool, VkCommandBufferLevel level, unsigned int nrCmdBuffers = 1,
							VkCommandBufferUsageFlags usage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
							VkCommandBufferInheritanceInfo *pInheritInfo = nullptr, const void *pNext = nullptr) {
		std::vector<VkCommandBuffer> cmd = allocateCommandBuffers(commandPool, level, nrCmdBuffers);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = pNext;
		beginInfo.flags = usage;
		beginInfo.pInheritanceInfo = pInheritInfo;

		vkBeginCommandBuffer(cmd[0], &beginInfo);

		return cmd;
	}

	void endSingleTimeCommands(VkQueue queue, VkCommandBuffer commandBuffer, VkCommandPool commandPool) {

		VkResult result = vkEndCommandBuffer(commandBuffer);

		// VkSubmitInfo submitInfo{};
		// submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		// submitInfo.commandBufferCount = 1;
		// submitInfo.pCommandBuffers = &commandBuffer;
		const std::vector<VkCommandBuffer> cmds = {commandBuffer};
		submitCommands(queue, cmds);
		// result =vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		result = vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(getHandle(), commandPool, cmds.size(), cmds.data());
	}

	/**
	 * @brief
	 *
	 * @param format
	 * @return true
	 * @return false
	 */
	bool isFormatSupported(VkFormat format, VkImageType imageType, VkImageTiling tiling,
						   VkImageUsageFlags usage) const noexcept {
		return this->physicalDevices[0]->isFormatSupported(format, imageType, tiling, usage);
	}

  private:
	uint32_t graphics_queue_node_index;
	uint32_t compute_queue_node_index;
	uint32_t transfer_queue_node_index;
	uint32_t present_queue_node_index;
	uint32_t sparse_queue_node_index;

	std::vector<std::shared_ptr<PhysicalDevice>> physicalDevices;
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;
	VkQueue sparseQueue;
};

#endif
