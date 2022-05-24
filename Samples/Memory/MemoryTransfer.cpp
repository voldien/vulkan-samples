#include "Importer/ImageImport.h"
#include "VKUtil.h"
#include "VksCommon.h"
#include <cxxopts.hpp>
#include <fmt/format.h>

static void bufferMemoryTransfer(std::shared_ptr<VKDevice> &device, VkQueue transfer, VkCommandBuffer cmd,
								 std::vector<int64_t> &timeSample, VkBuffer srcBuffer, VkBuffer dstBuffer,
								 VkDeviceSize memorySize) {

	VkCommandBufferBeginInfo cmdBufBeginInfo{};

	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = 0;

	for (size_t n = 0; n < timeSample.size(); n++) {

		VkBufferCopy bufferCopy;
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset = 0;
		bufferCopy.size = memorySize;

		vkBeginCommandBuffer(cmd, &cmdBufBeginInfo);
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &bufferCopy);
		vkEndCommandBuffer(cmd);
		/*	*/
		uint64_t timeStamp = SDL_GetPerformanceCounter();
		device->submitCommands(transfer, {cmd}, {}, {}, VK_NULL_HANDLE, {});
		/*	Start time.	*/
		VKS_VALIDATE(vkQueueWaitIdle(transfer));
		/*	End Timer.	*/
		uint64_t elapsedTime = SDL_GetPerformanceCounter() - timeStamp;
		timeSample[n] = elapsedTime;
		vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}
	float averageTime = 0;
	for (size_t n = 0; n < timeSample.size(); n++) {
		averageTime += static_cast<float>(timeSample[n]) / static_cast<float>(SDL_GetPerformanceFrequency());
	}

	averageTime /= timeSample.size();
	float totalTransferRate = (1.0 / averageTime) * memorySize;
	std::string resultMsg =
		fmt::format("{} KB - average: {} secs - ( Average Transfer Rate {} MB/s) (samples: {})", memorySize / 1024,
					averageTime, totalTransferRate / (1024 * 1024), timeSample.size());
	std::cout << resultMsg << std::endl;
}

class MemoryTransfer : public VKSampleSession {
  public:
	MemoryTransfer(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKSampleSession(core, device) {}

	virtual void run() override {
		const int nrTransferSamples = 100;

		/*	1KB, 1MB, 128MB, 512MB.	*/
		const std::array<VkDeviceSize, 4> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 128, 1024 * 1024 * 512};

		std::vector<int64_t> timeSample(nrTransferSamples);

		std::vector<VkBuffer> stagingBuffer(memorySizes.size());
		std::vector<VkBuffer> cpu2gpuBuffer(memorySizes.size());
		std::vector<VkBuffer> gpu2gpuBufferSrc(memorySizes.size());
		std::vector<VkBuffer> gpu2gpuBufferDst(memorySizes.size());
		std::vector<VkBuffer> gpu2cpuBufferSrc(memorySizes.size());
		std::vector<VkBuffer> gpu2cpuBufferDst(memorySizes.size());

		std::vector<VkDeviceMemory> staging(memorySizes.size());
		std::vector<VkDeviceMemory> cpu2gpu(memorySizes.size());
		std::vector<VkDeviceMemory> gpu2gpu_src(memorySizes.size());
		std::vector<VkDeviceMemory> gpu2gpu_dst(memorySizes.size());
		std::vector<VkDeviceMemory> gpu2cpu_src(memorySizes.size());
		std::vector<VkDeviceMemory> gpu2cpu_dst(memorySizes.size());

		try {
			VkQueue transfer = getVKDevice()->getDefaultTransfer();

			const VkPhysicalDeviceMemoryProperties &memProp = device->getPhysicalDevices()[0]->getMemoryProperties();

			VkCommandPool commandPool = device->createCommandPool(device->getDefaultTransferQueueIndex());
			std::vector<VkCommandBuffer> cmds =
				device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

			/*	CPU->GPU	*/
			/*	Allocate all buffers.	*/
			for (size_t i = 0; i < memorySizes.size(); i++) {
				VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
									   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer[i], staging[i]);

				VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
									   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cpu2gpuBuffer[i], cpu2gpu[i]);
			}
			std::cout << std::endl << "CPU to GPU Buffer Memory Transfer Speed" << std::endl;
			for (size_t i = 0; i < memorySizes.size(); i++) {
				bufferMemoryTransfer(device, transfer, cmds[0], timeSample, stagingBuffer[i], cpu2gpuBuffer[i],
									 memorySizes[i]);
			}

			for (size_t i = 0; i < memorySizes.size(); i++) {
				vkDestroyBuffer(device->getHandle(), stagingBuffer[i], nullptr);
				vkFreeMemory(device->getHandle(), staging[i], nullptr);
				vkDestroyBuffer(device->getHandle(), cpu2gpuBuffer[i], nullptr);
				vkFreeMemory(device->getHandle(), cpu2gpu[i], nullptr);
			}

			/*	Allocate all buffers.	*/
			for (size_t i = 0; i < memorySizes.size(); i++) {

				/*	*/
				VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2gpuBufferSrc[i], gpu2gpu_src[i]);
				VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2gpuBufferDst[i], gpu2gpu_dst[i]);
			}

			/*	GPU->GPU	*/
			std::cout << std::endl << "GPU to GPU Buffer Memory Transfer Speed" << std::endl;
			for (size_t i = 0; i < memorySizes.size(); i++) {
				bufferMemoryTransfer(device, transfer, cmds[0], timeSample, gpu2gpuBufferSrc[i], gpu2gpuBufferDst[i],
									 memorySizes[i]);
			}

			for (size_t i = 0; i < memorySizes.size(); i++) {
				vkDestroyBuffer(device->getHandle(), gpu2gpuBufferSrc[i], nullptr);
				vkFreeMemory(device->getHandle(), gpu2gpu_src[i], nullptr);
				vkDestroyBuffer(device->getHandle(), gpu2gpuBufferDst[i], nullptr);
				vkFreeMemory(device->getHandle(), gpu2gpu_dst[i], nullptr);
			}

			/*	Allocate all buffers.	*/
			for (size_t i = 0; i < memorySizes.size(); i++) {
				VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu2cpuBufferSrc[i], gpu2cpu_src[i]);
				VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, gpu2cpuBufferDst[i], gpu2cpu_dst[i]);
			}

			/*	GPU->CPU	*/
			std::cout << std::endl << "GPU to CPU Buffer Memory Transfer Speed" << std::endl;
			for (size_t i = 0; i < memorySizes.size(); i++) {
				bufferMemoryTransfer(device, transfer, cmds[0], timeSample, gpu2cpuBufferSrc[i], gpu2cpuBufferDst[i],
									 memorySizes[i]);
			}

			vkFreeCommandBuffers(device->getHandle(), commandPool, cmds.size(), cmds.data());
			vkDestroyCommandPool(device->getHandle(), commandPool, nullptr);

			/*	Release buffers.	*/
			for (size_t i = 0; i < memorySizes.size(); i++) {
				vkDestroyBuffer(device->getHandle(), gpu2cpuBufferSrc[i], nullptr);
				vkFreeMemory(device->getHandle(), gpu2cpu_src[i], nullptr);
				vkDestroyBuffer(device->getHandle(), gpu2cpuBufferDst[i], nullptr);
				vkFreeMemory(device->getHandle(), gpu2cpu_dst[i], nullptr);
			}
		} catch (const std::exception &ex) {
			std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		}
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, false},
																		   {"VK_KHR_xlib_surface", false}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, false}};

	try {
		VKSampleWindow<MemoryTransfer> mandel(argc, argv, required_device_extensions, {}, required_instance_extensions);
		mandel.run();

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}