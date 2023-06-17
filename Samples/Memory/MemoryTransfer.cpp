#include "vulkan/vulkan_core.h"
#include <Importer/ImageImport.h>
#include <VKSample.h>
#include <VKSampleBase.h>
#include <VKUtil.h>
#include <cstdint>
#include <cxxopts.hpp>
#include <fmt/format.h>

namespace vksample {

	static void bufferMemoryTransfer(std::shared_ptr<VKDevice> &device, VkQueue transferQueue, VkCommandBuffer cmd,
									 std::vector<int64_t> &timeSampleNanoSeconds, VkBuffer srcBuffer,
									 VkBuffer dstBuffer, VkDeviceSize sampleMemorySize, VkQueryPool queryPool) {

		VkCommandBufferBeginInfo cmdBufBeginInfo{};

		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = 0;

		/*	*/
		// TODO Fix how to handle that correct device is selected.
		const float timestampPeriod = device->getPhysicalDevices()[0]->getDeviceLimits().timestampPeriod;

		for (size_t n = 0; n < timeSampleNanoSeconds.size(); n++) {

			VkBufferCopy bufferCopy;
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset = 0;
			bufferCopy.size = sampleMemorySize;

			vkBeginCommandBuffer(cmd, &cmdBufBeginInfo);
			vkCmdResetQueryPool(cmd, queryPool, 0, 2);
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, 0);
			vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &bufferCopy);
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
			vkEndCommandBuffer(cmd);

			/*	*/
			device->submitCommands(transferQueue, {cmd}, {}, {}, VK_NULL_HANDLE, {});

			/*	Wait intill all commands are complete.	*/
			VKS_VALIDATE(vkQueueWaitIdle(transferQueue));

			uint64_t buffer[2];
			VkResult result =
				vkGetQueryPoolResults(device->getHandle(), queryPool, 0, 2, sizeof(uint64_t) * 2, buffer,
									  sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
			if (!(result == VK_SUCCESS || result == VK_NOT_READY)) {
				VKS_VALIDATE(result);
			}

			/*	Last - First Timestamp, multiplied by timstamp period time.	*/
			timeSampleNanoSeconds[n] = (buffer[1] - buffer[0]) * timestampPeriod;
			assert(timeSampleNanoSeconds[n] >= 0);

			VKS_VALIDATE(vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
		}

		/*	Sum all average times. 1/N * SUM(samples)*/
		double averageTimeInSeconds = 0;
		for (size_t n = 0; n < timeSampleNanoSeconds.size(); n++) {
			averageTimeInSeconds += static_cast<double>(timeSampleNanoSeconds[n]) / static_cast<double>(10e9);
		}
		averageTimeInSeconds *= 1.0 / static_cast<double>(timeSampleNanoSeconds.size());

		/*	Total amount of data sent.	*/
		const uint64_t totalTransferbyte = timeSampleNanoSeconds.size() * sampleMemorySize;

		double averageTransferPerSecond = (sampleMemorySize / averageTimeInSeconds);
		averageTransferPerSecond = averageTransferPerSecond / (1024.0 * 1024.0);

		std::string resultMsg = fmt::format(
			"{} KB - average: {:.8f} secs - ( Average Transfer Rate {:.2f} MB/s) (samples: {})",
			sampleMemorySize / 1024, averageTimeInSeconds, averageTransferPerSecond, timeSampleNanoSeconds.size());
		std::cout << resultMsg << std::endl;
	}

	class MemoryTransfer : public vkscommon::VKSampleSessionBase {
	  private:
		VkQueryPool queryPool;

		/*	1KB, 1MB, 128MB, 512MB, 1024MB	*/
		const std::array<VkDeviceSize, 5> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 128, 1024 * 1024 * 512,
														 1 * 1024 * 1024};

		std::vector<VkBuffer> sourceBuffer = std::vector<VkBuffer>(memorySizes.size(), VK_NULL_HANDLE);
		std::vector<VkBuffer> destinationBuffer = std::vector<VkBuffer>(memorySizes.size(), VK_NULL_HANDLE);

		std::vector<VkDeviceMemory> sourceMemory = std::vector<VkDeviceMemory>(memorySizes.size(), VK_NULL_HANDLE);
		std::vector<VkDeviceMemory> targetMemory = std::vector<VkDeviceMemory>(memorySizes.size(), VK_NULL_HANDLE);

	  public:
		MemoryTransfer(std::shared_ptr<fvkcore::VulkanCore> &core, std::shared_ptr<fvkcore::VKDevice> &device)
			: VKSampleSessionBase(core, device) {
			this->transfer_queue = device->getQueue(0, 0);
		}

		virtual void release() override {
			this->releaseMemory();
			vkDestroyQueryPool(this->getDevice(), this->queryPool, nullptr);
		}

		virtual void loadDefaultQueue() override {}

		void releaseMemory() {

			for (size_t i = 0; i < this->memorySizes.size(); i++) {
				vkDestroyBuffer(this->device->getHandle(), this->sourceBuffer[i], nullptr);
				vkFreeMemory(this->device->getHandle(), this->sourceMemory[i], nullptr);
				vkDestroyBuffer(this->device->getHandle(), this->destinationBuffer[i], nullptr);
				vkFreeMemory(this->device->getHandle(), this->targetMemory[i], nullptr);
			}

			this->sourceBuffer = std::vector<VkBuffer>(memorySizes.size(), VK_NULL_HANDLE);
			this->destinationBuffer = std::vector<VkBuffer>(memorySizes.size(), VK_NULL_HANDLE);

			this->sourceMemory = std::vector<VkDeviceMemory>(memorySizes.size(), VK_NULL_HANDLE);
			this->targetMemory = std::vector<VkDeviceMemory>(memorySizes.size(), VK_NULL_HANDLE);
		}

		virtual void run() override {
			const int nrTransferSamples = 150; // TODO ass argument.

			VkQueryPoolCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;

			createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			createInfo.queryCount = 2; // TODO

			VKS_VALIDATE(vkCreateQueryPool(this->getDevice(), &createInfo, nullptr, &queryPool));

			// TODO find the queue with best and possbile timestamp capability.
			// TODO check if any of the queue supports timestamp, by looking at timestampValidBits value.

			std::vector<int64_t> timeSample(nrTransferSamples);

			// TODO add support to look at the heap.

			try {
				VkQueue transfer = this->getDefaultTransferQueue();

				const VkPhysicalDeviceMemoryProperties &memProp =
					this->getVKDevice()->getPhysicalDevices()[0]->getMemoryProperties();

				VkCommandPool commandPool =
					this->getVKDevice()->createCommandPool(this->getDefaultTransferQueueIndex());
				std::vector<VkCommandBuffer> cmds =
					device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

				/*	CPU->GPU	*/
				/*	Allocate all buffers.	*/
				for (size_t i = 0; i < memorySizes.size(); i++) {

					VKHelper::createBuffer(this->getVKDevice()->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   this->sourceBuffer[i], this->sourceMemory[i]);

					VKHelper::createBuffer(this->getVKDevice()->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
										   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, destinationBuffer[i], targetMemory[i]);
				}
				std::cout << std::endl << "CPU to GPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(this->getVKDevice(), transfer, cmds[0], timeSample, sourceBuffer[i],
										 destinationBuffer[i], memorySizes[i], this->queryPool);
				}

				this->releaseMemory();

				/*	Allocate all buffers.	*/
				for (size_t i = 0; i < memorySizes.size(); i++) {

					/*	*/
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										   sourceBuffer[i], sourceMemory[i]);
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										   destinationBuffer[i], targetMemory[i]);
				}

				/*	GPU->GPU	*/
				std::cout << std::endl << "GPU to GPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(device, transfer, cmds[0], timeSample, sourceBuffer[i], destinationBuffer[i],
										 memorySizes[i], this->queryPool);
				}

				this->releaseMemory();

				/*	Allocate all buffers.	*/
				for (size_t i = 0; i < memorySizes.size(); i++) {
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										   sourceBuffer[i], sourceMemory[i]);
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   destinationBuffer[i], targetMemory[i]);
				}

				/*	GPU->CPU	*/
				std::cout << std::endl << "GPU to CPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(device, transfer, cmds[0], timeSample, sourceBuffer[i], destinationBuffer[i],
										 memorySizes[i], this->queryPool);
				}

				/*	Release buffers.	*/
				this->releaseMemory();

				/*	Allocate all buffers.	*/
				for (size_t i = 0; i < memorySizes.size(); i++) {
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   sourceBuffer[i], sourceMemory[i]);
					VKHelper::createBuffer(device->getHandle(), memorySizes[i], memProp,
										   VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
										   destinationBuffer[i], targetMemory[i]);
				}

				/*	CPU->CPU	*/
				std::cout << std::endl << "CPU to CPU Buffer Memory Transfer Speed" << std::endl;
				for (size_t i = 0; i < memorySizes.size(); i++) {
					bufferMemoryTransfer(device, transfer, cmds[0], timeSample, sourceBuffer[i], destinationBuffer[i],
										 memorySizes[i], this->queryPool);
				}

				/*	Release buffers.	*/
				this->releaseMemory();

				vkFreeCommandBuffers(device->getHandle(), commandPool, cmds.size(), cmds.data());
				vkDestroyCommandPool(device->getHandle(), commandPool, nullptr);

			} catch (const std::exception &ex) {
				std::cerr << "Failed During Benchmark" << std::endl;
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

		std::vector<VkDeviceQueueCreateInfo>
		OnSelectQueue(std::vector<std::shared_ptr<PhysicalDevice>> &physical_devices) override {
			std::vector<VkDeviceQueueCreateInfo> queues;

			uint32_t timestampvalid = 0;
			int queueIndex = -1;
			for (size_t j = 0; j < physical_devices[0]->getQueueFamilyProperties().size(); j++) {
				/*  */
				const VkQueueFamilyProperties &familyProp = physical_devices[0]->getQueueFamilyProperties()[j];
				if ((familyProp.queueFlags & VK_QUEUE_TRANSFER_BIT) && familyProp.timestampValidBits > timestampvalid) {
					timestampvalid = familyProp.timestampValidBits;
					queueIndex = j;
				}
			}
			std::vector<float> queuePriorities(1, 1.0f);

			VkDeviceQueueCreateInfo queueCreateInfo;
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pNext = nullptr;
			queueCreateInfo.flags = 0;
			queueCreateInfo.queueFamilyIndex = queueIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();

			queues.push_back(queueCreateInfo);
			return queues;
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