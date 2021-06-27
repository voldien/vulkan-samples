#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VksCommon.h"
#include "common.hpp"
#include <SDL2/SDL.h>

int main(int argc, const char **argv) {

	const int nrTransferSamples = 100;
	const int roundRobin = 3;

	const std::array<size_t, 3> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 1024};

	std::vector<int> timeSample(nrTransferSamples);

	std::vector<VkBuffer> stagingBuffer(roundRobin);
	std::vector<VkBuffer> cpu2gpuBuffer(roundRobin);
	std::vector<VkBuffer> gpu2gpuBuffer(roundRobin);
	std::vector<VkBuffer> gpu2cpuBuffer(roundRobin);

	std::vector<VkDeviceMemory> staging(roundRobin);
	std::vector<VkDeviceMemory> cpu2gpu(roundRobin);
	std::vector<VkDeviceMemory> gpu2gpu(roundRobin);
	std::vector<VkDeviceMemory> gpu2cpu(roundRobin);

	std::unordered_map<const char *, bool> required_device_extensions = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> phyDevices = core->createPhysicalDevices();
		std::shared_ptr<VKDevice> device = std::make_shared<VKDevice>(phyDevices);

		VkQueue transfer = device->getDefaultTransfer();

		VkQueryPool queryPool;

		VkQueryPoolCreateInfo queryInfo{};
		queryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryInfo.pNext = nullptr;
		queryInfo.flags = 0;
		queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryInfo.queryCount = memorySizes.size() * roundRobin;
		queryInfo.pipelineStatistics = 0;

		vkCreateQueryPool(device->getHandle(), &queryInfo, nullptr, &queryPool);
		vkResetQueryPool(device->getHandle(), queryPool, 0, memorySizes.size() * roundRobin);

		// this->getLogicalDevice()->get
		const VkPhysicalDeviceMemoryProperties &memProp = device->getPhysicalDevices()[0]->getMemoryProperties();
		VkDeviceSize bufferSize = 1024;

		VkCommandPool commandPool = device->createCommandPool(device->getDefaultTransferQueueIndex());
		std::vector<VkCommandBuffer> cmds = device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
																		   memorySizes.size() * roundRobin);

		for (int i = 0; i < roundRobin; i++) {

		}

		/*	CPU->GPU	*/
		for (int i = 0; i < roundRobin; i++) {
			vkCmdBeginQuery(cmds[i], queryPool, i, VK_QUERY_CONTROL_PRECISE_BIT);
			//vkFlushMappedMemoryRanges
			//vkCmdCopyBuffer()
			// VKHelper::createMemory(device->getPhysicalDevice(0), bufferSize,
			// device->getPhysicalDevice(0)->getMemoryProperties(), )
			vkCmdWriteTimestamp(cmds[i], VK_PIPELINE_STAGE_TRANSFER_BIT, queryPool, i);
		}
		vkGetQueryPoolResults(device->getHandle(), queryPool, 0, 9, timeSample.size() * sizeof(int), timeSample.data(), 0, VK_QUERY_RESULT_64_BIT);

		device->submitCommands(transfer, cmds);
		vkQueueWaitIdle(transfer);

		/*	GPU->GPU	*/

		/*	GPU->CPU	*/

		/*	*/
		for (int i = 0; i < memProp.memoryHeapCount; i++) {
			if (memProp.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				//				VKHelper::createBuffer(getDevice(), bufferSize, memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				//memProp.memoryHeaps[i].flags, memo )
			}
		}

	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}