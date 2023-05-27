#include <Importer/ImageImport.h>
#include <VKSample.h>
#include <VKSampleBase.h>
#include <VKUtil.h>
#include <cxxopts.hpp>
#include <fmt/format.h>
namespace vksample {

	static void bufferMemoryTransfer(std::shared_ptr<VKDevice> &device, VkQueue transferQueue, VkCommandBuffer cmd,
									 std::vector<int64_t> &timeSample, VkBuffer srcBuffer, VkBuffer dstBuffer,
									 VkDeviceSize sampleMemorySize, VkQueryPool queryPool) {

		VkCommandBufferBeginInfo cmdBufBeginInfo{};

		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = 0;

		/*	*/
		const size_t timestampPeriod = device->getPhysicalDevices()[0]->getDeviceLimits().timestampPeriod;

		for (size_t n = 0; n < timeSample.size(); n++) {

			VkBufferCopy bufferCopy;
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset = 0;
			bufferCopy.size = sampleMemorySize;

			vkBeginCommandBuffer(cmd, &cmdBufBeginInfo);
			vkCmdResetQueryPool(cmd, queryPool, 0, 2);
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, queryPool, 0);
			vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &bufferCopy);
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
			vkEndCommandBuffer(cmd);

			/*	*/
			device->submitCommands(transferQueue, {cmd}, {}, {}, VK_NULL_HANDLE, {});
			/*	Start time.	*/
			VKS_VALIDATE(vkQueueWaitIdle(transferQueue));

			uint64_t buffer[2];
			VkResult result = vkGetQueryPoolResults(device->getHandle(), queryPool, 0, 2, sizeof(uint64_t) * 2, buffer,
													sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

			timeSample[n] = (buffer[1] - buffer[0]);

			VKS_VALIDATE(vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
		}

		float averageTime = 0;
		for (size_t n = 0; n < timeSample.size(); n++) {
			averageTime += static_cast<float>(timeSample[n] * timestampPeriod) / static_cast<float>(10e9);
		}

		averageTime /= static_cast<float>(timeSample.size());

		float totalTransferRate = (1.0 / averageTime) * sampleMemorySize;
		std::string resultMsg =
			fmt::format("{} KB - average: {} secs - ( Average Transfer Rate {} MB/s) (samples: {})",
						sampleMemorySize / 1024, averageTime, totalTransferRate / (1024 * 1024), timeSample.size());
		std::cout << resultMsg << std::endl;
	}

	class MemoryTransfer : public vkscommon::VKSampleSessionBase {
	  private:
		VkQueryPool queryPool;

	  public:
		MemoryTransfer(std::shared_ptr<fvkcore::VulkanCore> &core, std::shared_ptr<fvkcore::VKDevice> &device)
			: VKSampleSessionBase(core, device) {}

		virtual void release() override { vkDestroyQueryPool(this->getDevice(), queryPool, nullptr); }

		virtual void run() override {
			const int nrTransferSamples = 100; // TODO ass argument.

			VkQueryPoolCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;

			createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			createInfo.queryCount = 2; // TODO

			VKS_VALIDATE(vkCreateQueryPool(this->getDevice(), &createInfo, nullptr, &queryPool));

			// TODO find the queue with best and possbile timestamp capability.

			/*	1KB, 1MB, 128MB, 512MB.	*/
			const std::array<VkDeviceSize, 5> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 128, 1024 * 1024 * 512,
															 1024 * 1024 * 1024};

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
				VkQueue transfer = this->getVKDevice()->getDefaultTransfer();

				const VkPhysicalDeviceMemoryProperties &memProp =
					this->getVKDevice()->getPhysicalDevices()[0]->getMemoryProperties();

				VkCommandPool commandPool =
					this->getVKDevice()->createCommandPool(device->getDefaultTransferQueueIndex());
				std::vector<VkCommandBuffer> cmds =
					device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

				/*	CPU->GPU	*/
				/*	Allocate all buffers.	*/
				for (size_t i = 0; i < memorySizes.size(); i++) {
					VKHelper::createBuffer(this->getVKDevice()->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   stagingBuffer[i], staging[i]);

					VKHelper::createBuffer(this->getVKDevice()->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
										   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cpu2gpuBuffer[i], cpu2gpu[i]);
				}
				std::cout << std::endl << "CPU to GPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(this->getVKDevice(), transfer, cmds[0], timeSample, stagingBuffer[i],
										 cpu2gpuBuffer[i], memorySizes[i], this->queryPool);
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
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										   gpu2gpuBufferSrc[i], gpu2gpu_src[i]);
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										   gpu2gpuBufferDst[i], gpu2gpu_dst[i]);
				}

				/*	GPU->GPU	*/
				std::cout << std::endl << "GPU to GPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(device, transfer, cmds[0], timeSample, gpu2gpuBufferSrc[i],
										 gpu2gpuBufferDst[i], memorySizes[i], this->queryPool);
				}

				for (size_t i = 0; i < memorySizes.size(); i++) {
					vkDestroyBuffer(device->getHandle(), gpu2gpuBufferSrc[i], nullptr);
					vkFreeMemory(device->getHandle(), gpu2gpu_src[i], nullptr);
					vkDestroyBuffer(device->getHandle(), gpu2gpuBufferDst[i], nullptr);
					vkFreeMemory(device->getHandle(), gpu2gpu_dst[i], nullptr);
				}

				/*	Allocate all buffers.	*/
				for (size_t i = 0; i < memorySizes.size(); i++) {
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										   gpu2cpuBufferSrc[i], gpu2cpu_src[i]);
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   gpu2cpuBufferDst[i], gpu2cpu_dst[i]);
				}

				/*	GPU->CPU	*/
				std::cout << std::endl << "GPU to CPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(device, transfer, cmds[0], timeSample, gpu2cpuBufferSrc[i],
										 gpu2cpuBufferDst[i], memorySizes[i], this->queryPool);
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

	class MemoryTransferVKSample : public VKSample<MemoryTransfer> {
	  public:
		MemoryTransferVKSample() : VKSample<MemoryTransfer>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			// options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"));
		}
	};
} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		vksample::MemoryTransferVKSample memoryTransfer;
		memoryTransfer.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}