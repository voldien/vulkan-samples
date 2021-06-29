#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VKUtil.h"
#include "VksCommon.h"


int main(int argc, const char **argv) {

	const int nrTransferSamples = 100;
	const int roundRobin = 3;

	/*	1KB, 1MB, 128MB, 512MB.	*/
	const std::array<VkDeviceSize, 4> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 128, 1024 * 1024 * 512};

	std::vector<int64_t> timeSample(nrTransferSamples);

	std::vector<VkBuffer> stagingBuffer(memorySizes.size());
	std::vector<VkBuffer> cpu2gpuBuffer(memorySizes.size());
	std::vector<VkBuffer> gpu2gpuBufferSrc(memorySizes.size());
	std::vector<VkBuffer> gpu2gpuBufferDst(memorySizes.size());
	std::vector<VkBuffer> gpu2cpuBuffer(memorySizes.size());

	std::vector<VkDeviceMemory> staging(memorySizes.size());
	std::vector<VkDeviceMemory> cpu2gpu(memorySizes.size());
	std::vector<VkDeviceMemory> gpu2gpu_src(memorySizes.size());
	std::vector<VkDeviceMemory> gpu2gpu_dst(memorySizes.size());
	std::vector<VkDeviceMemory> gpu2cpu(memorySizes.size());

	std::unordered_map<const char *, bool> required_device_extensions = {{"VK_KHR_performance_query", false}};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> phyDevices = core->createPhysicalDevices();
		std::shared_ptr<VKDevice> device = std::make_shared<VKDevice>(phyDevices, required_device_extensions);

		VkQueue transfer = device->getDefaultTransfer();

		const VkPhysicalDeviceMemoryProperties &memProp = device->getPhysicalDevices()[0]->getMemoryProperties();

		VkCommandPool commandPool = device->createCommandPool(device->getDefaultTransferQueueIndex());
		std::vector<VkCommandBuffer> cmds = device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
																		   memorySizes.size() * roundRobin);

		/*	Allocate all buffers.	*/
		for (int i = 0; i < memorySizes.size(); i++) {
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer[i], staging[i]);

			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
								   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cpu2gpuBuffer[i], cpu2gpu[i]);
		}

		/*	CPU->GPU	*/
		VkCommandBufferBeginInfo cmdBufBeginInfo{};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = 0;

		std::cout << "CPU to GPU Memory Transfer Speed" << std::endl;
		for (size_t i = 0; i < memorySizes.size(); i++) {

			for (size_t n = 0; n < timeSample.size(); n++) {

				VkBufferCopy bufferCopy;
				bufferCopy.dstOffset = 0;
				bufferCopy.srcOffset = 0;
				bufferCopy.size = memorySizes[i];

				vkBeginCommandBuffer(cmds[0], &cmdBufBeginInfo);
				vkCmdCopyBuffer(cmds[0], stagingBuffer[i], cpu2gpuBuffer[i], 1, &bufferCopy);
				vkEndCommandBuffer(cmds[0]);
				/*	*/
				uint64_t timeStamp = SDL_GetPerformanceCounter();
				device->submitCommands(transfer, {cmds[0]}, {}, {}, VK_NULL_HANDLE, {});
				/*	Start time.	*/
				VKS_VALIDATE(vkQueueWaitIdle(transfer));
				/*	End Timer.	*/
				uint64_t elapsedTime = SDL_GetPerformanceCounter() - timeStamp;
				timeSample[n] = elapsedTime;
				vkResetCommandBuffer(cmds[0], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			}
			float averageTime = 0;
			for (int n = 0; n < timeSample.size(); n++) {
				averageTime += (float)timeSample[n] / (float)SDL_GetPerformanceFrequency();
			}
			averageTime /= timeSample.size();

			std::cout << memorySizes[i] / 1024 << " KB " << averageTime << std::endl;
		}

		for (int i = 0; i < memorySizes.size(); i++) {
			vkDestroyBuffer(device->getHandle(), stagingBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), staging[i], nullptr);
			vkDestroyBuffer(device->getHandle(), cpu2gpuBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), cpu2gpu[i], nullptr);
		}

		/*	Allocate all buffers.	*/
		for (int i = 0; i < memorySizes.size(); i++) {

			/*	*/
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2gpuBufferSrc[i], gpu2gpu_src[i]);
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2gpuBufferDst[i], gpu2gpu_dst[i]);
		}

		/*	GPU->GPU	*/
		std::cout << "GPU to GPU Memory Transfer Speed" << std::endl;
		for (int i = 0; i < memorySizes.size(); i++) {

			for (int n = 0; n < timeSample.size(); n++) {

				VkBufferCopy bufferCopy;
				bufferCopy.dstOffset = 0;
				bufferCopy.srcOffset = 0;
				bufferCopy.size = memorySizes[i];

				vkBeginCommandBuffer(cmds[0], &cmdBufBeginInfo);
				vkCmdCopyBuffer(cmds[0], gpu2gpuBufferSrc[i], gpu2gpuBufferDst[i], 1, &bufferCopy);
				vkEndCommandBuffer(cmds[0]);
				/*	*/
				uint64_t timeStamp = SDL_GetPerformanceCounter();
				device->submitCommands(transfer, {cmds[0]}, {}, {}, VK_NULL_HANDLE, {});
				/*	Start time.	*/
				VKS_VALIDATE(vkQueueWaitIdle(transfer));
				/*	End Timer.	*/
				uint64_t elapsedTime = SDL_GetPerformanceCounter() - timeStamp;
				timeSample[n] = elapsedTime;
				vkResetCommandBuffer(cmds[0], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			}
			float averageTime = 0;
			for (int n = 0; n < timeSample.size(); n++) {
				averageTime += (float)timeSample[n] / (float)SDL_GetPerformanceFrequency();
			}
			averageTime /= timeSample.size();

			std::cout << memorySizes[i] / 1024 << " KB " << averageTime << std::endl;
		}

		for (int i = 0; i < memorySizes.size(); i++) {
			vkDestroyBuffer(device->getHandle(), gpu2gpuBufferSrc[i], nullptr);
			vkFreeMemory(device->getHandle(), gpu2gpu_src[i], nullptr);
			vkDestroyBuffer(device->getHandle(), gpu2gpuBufferDst[i], nullptr);
			vkFreeMemory(device->getHandle(), gpu2gpu_dst[i], nullptr);
		}

		/*	Allocate all buffers.	*/
		for (int i = 0; i < memorySizes.size(); i++) {
			VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2cpuBuffer[i], gpu2cpu[i]);
		}

		/*	GPU->CPU	*/
		std::cout << "GPU to CPU Memory Transfer Speed" << std::endl;
		// for (int i = 0; i < memorySizes.size(); i++) {

		// 	for (int n = 0; n < timeSample.size(); n++) {

		// 		VkBufferCopy bufferCopy;
		// 		bufferCopy.dstOffset = 0;
		// 		bufferCopy.srcOffset = 0;
		// 		bufferCopy.size = memorySizes[i];

		// 		vkBeginCommandBuffer(cmds[0], &cmdBufBeginInfo);
		// 		vkCmdCopyBuffer(cmds[0], gpu2cpuBuffer[i], stagingBuffer[i], 1, &bufferCopy);
		// 		vkEndCommandBuffer(cmds[0]);
		// 		/*	*/
		// 		uint64_t timeStamp = SDL_GetPerformanceCounter();
		// 		device->submitCommands(transfer, {cmds[0]}, {}, {}, VK_NULL_HANDLE, {});
		// 		/*	Start time.	*/
		// 		VKS_VALIDATE(vkQueueWaitIdle(transfer));
		// 		/*	End Timer.	*/
		// 		uint64_t elapsedTime = SDL_GetPerformanceCounter() - timeStamp;
		// 		timeSample[n] = elapsedTime;
		// 		vkResetCommandBuffer(cmds[0], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		// 	}
		// 	float averageTime = 0;
		// 	for (int n = 0; n < timeSample.size(); n++) {
		// 		averageTime += (float)timeSample[n] / (float)SDL_GetPerformanceFrequency();
		// 	}
		// 	averageTime /= timeSample.size();

		// 	std::cout << memorySizes[i] / 1024 << " KB " << averageTime << std::endl;
		// }

		vkFreeCommandBuffers(device->getHandle(), commandPool, cmds.size(), cmds.data());
		vkDestroyCommandPool(device->getHandle(), commandPool, nullptr);

		/*	Release buffers.	*/
		for (int i = 0; i < memorySizes.size(); i++) {
			vkDestroyBuffer(device->getHandle(), gpu2cpuBuffer[i], nullptr);
			vkFreeMemory(device->getHandle(), gpu2cpu[i], nullptr);
		}
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}