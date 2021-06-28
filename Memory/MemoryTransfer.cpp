#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VKUtil.h"
#include "VksCommon.h"
#include "common.hpp"

int main(int argc, const char **argv) {

	const int nrTransferSamples = 100;
	const int roundRobin = 3;

	const std::array<size_t, 3> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 1024};

	std::vector<int64_t> timeSample(nrTransferSamples);

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

		// this->getLogicalDevice()->get
		const VkPhysicalDeviceMemoryProperties &memProp = device->getPhysicalDevices()[0]->getMemoryProperties();
		VkDeviceSize bufferSize = 1024;

		VkCommandPool commandPool = device->createCommandPool(device->getDefaultTransferQueueIndex());
		std::vector<VkCommandBuffer> cmds = device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
																		   memorySizes.size() * roundRobin);

		/*	*/
		for (int i = 0; i < roundRobin; i++) {
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer[i], staging[i]);

			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, cpu2gpuBuffer[i], cpu2gpu[i]);
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2gpuBuffer[i], gpu2gpu[i]);
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2cpuBuffer[i], gpu2cpu[i]);
		}

		/*	CPU->GPU	*/
		VkCommandBufferBeginInfo cmdBufBeginInfo{};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = 0;

		vkBeginCommandBuffer(cmds[0], &cmdBufBeginInfo);
		vkCmdResetQueryPool(cmds[0], queryPool, 0, memorySizes.size() * roundRobin);

		VkBufferCopy bufferCopy;
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset = 0;
		bufferCopy.size = bufferSize;

		for (int i = 0; i < roundRobin; i++) {
			vkCmdBeginQuery(cmds[0], queryPool, i, 0);
			vkCmdWriteTimestamp(cmds[0], VK_PIPELINE_STAGE_HOST_BIT, queryPool, i);
			vkCmdCopyBuffer(cmds[0], stagingBuffer[i], cpu2gpuBuffer[i], 1, &bufferCopy);
			vkCmdEndQuery(cmds[0], queryPool, i);
		}

		vkEndCommandBuffer(cmds[0]);
		device->submitCommands(transfer, {cmds[0]});
		VKS_VALIDATE(vkQueueWaitIdle(transfer));

		VKS_VALIDATE(vkGetQueryPoolResults(device->getHandle(), queryPool, 0, roundRobin, timeSample.size() * sizeof(int64_t),
										   timeSample.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
										   printf("\n%ld, %ld, %ld\n", timeSample[0], timeSample[1], timeSample[1]);
		/*	GPU->GPU	*/

		/*	GPU->CPU	*/

		/*	*/
		for (unsigned int i = 0; i < memProp.memoryHeapCount; i++) {
			if (memProp.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				//				VKHelper::createBuffer(getDevice(), bufferSize, memProp,
				// VK_BUFFER_USAGE_TRANSFER_DST_BIT, memProp.memoryHeaps[i].flags, memo )
			}
		}

		/*	Release buffers.	*/
		for (int i = 0; i < roundRobin; i++) {
			vkDestroyBuffer(device->getHandle(), stagingBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), staging[i], nullptr);
			vkDestroyBuffer(device->getHandle(), cpu2gpuBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), cpu2gpu[i], nullptr);
			vkDestroyBuffer(device->getHandle(), gpu2gpuBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), gpu2gpu[i], nullptr);
			vkDestroyBuffer(device->getHandle(), gpu2cpuBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), gpu2cpu[i], nullptr);
		}

	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}