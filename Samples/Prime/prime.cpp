#include "vulkan/vulkan_core.h"
#include <VKSample.h>
#include <VKSampleBase.h>
#include <VKUtil.h>
#include <cstdint>
#include <cxxopts.hpp>
#include <fmt/format.h>

namespace vksample {

	class Prime : public vksample::VKSampleSessionBase {
	  private:
		/*	1KB, 1MB, 128MB, 512MB, 1024MB	*/
		const std::array<VkDeviceSize, 5> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 128, 1024 * 1024 * 512,
														 1 * 1024 * 1024};

		VkPipeline computeWilsomPrimePipeline = VK_NULL_HANDLE;
		VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descpool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> descriptorSets;

		std::vector<VkBuffer> sourceBuffer = std::vector<VkBuffer>(memorySizes.size(), VK_NULL_HANDLE);
		std::vector<VkBuffer> destinationBuffer = std::vector<VkBuffer>(memorySizes.size(), VK_NULL_HANDLE);

		std::vector<VkDeviceMemory> sourceMemory = std::vector<VkDeviceMemory>(memorySizes.size(), VK_NULL_HANDLE);
		std::vector<VkDeviceMemory> targetMemory = std::vector<VkDeviceMemory>(memorySizes.size(), VK_NULL_HANDLE);

	  public:
		Prime(std::shared_ptr<fvkcore::VulkanCore> &core, std::shared_ptr<fvkcore::VKDevice> &device)
			: VKSampleSessionBase(core, device) {
			this->transfer_queue = device->getQueue(0, 0);
		}

		const std::string computeGameOfLifeShaderPath = "Shaders/prime/wilsom.comp.spv";

		void release() override { this->releaseMemory(); }

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

		VkPipeline createComputePipeline(VkPipelineLayout *layout) {
			VkPipeline pipeline = nullptr;

			const auto compShaderCode =
				vksample::IOUtil::readFileData<uint32_t>(this->computeGameOfLifeShaderPath, this->getFileSystem());

			VkShaderModule compShaderModule = VKHelper::createShaderModule(this->getDevice(), compShaderCode);

			VkPipelineShaderStageCreateInfo compShaderStageInfo{};
			compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			compShaderStageInfo.module = compShaderModule;
			compShaderStageInfo.pName = "main";

			std::array<VkDescriptorSetLayoutBinding, 3> uboLayoutBindings{};

			/*	Render Texture.	*/
			uboLayoutBindings[2].binding = 0;
			uboLayoutBindings[2].descriptorCount = 1;
			uboLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uboLayoutBindings[2].pImmutableSamplers = nullptr;
			uboLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			/*	*/
			VKHelper::createDescriptorSetLayout(this->getDevice(), descriptorSetLayout, uboLayoutBindings);

			/*	*/
			VKHelper::createPipelineLayout(this->getDevice(), *layout, {descriptorSetLayout});

			pipeline = VKHelper::createComputePipeline(this->getDevice(), *layout, compShaderStageInfo);

			vkDestroyShaderModule(this->getDevice(), compShaderModule, nullptr);

			return pipeline;
		}

		 void Initialize() override {}

		 void run() override {

			const size_t primeCandidate = 1000;

			/*	Create pipeline.	*/
			this->computeWilsomPrimePipeline = createComputePipeline(&computePipelineLayout);

			const size_t nrBuf = 3;
			/*	Allocate descriptor set.	*/
			const std::vector<VkDescriptorPoolSize> poolSize = {{
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				static_cast<uint32_t>(nrBuf * 1), /*	3 storage image for each chain.	*/
			}};

			/*	*/
			this->descpool = VKHelper::createDescPool(getDevice(), poolSize, nrBuf);

			/*	*/
			VKS_VALIDATE(vkResetDescriptorPool(this->getDevice(), descpool, 0));

			std::vector<VkDescriptorSetLayout> layouts(3, descriptorSetLayout);
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = descpool; // pool to allocate from.
			descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(3);
			descriptorSetAllocateInfo.pSetLayouts = layouts.data();

			// allocate descriptor set.
			descriptorSets.resize(3);
			// vkFreeDescriptorSets
			VKS_VALIDATE(
				vkAllocateDescriptorSets(this->getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

			try {

				// TODO add support to look at the heap.
				VkQueue transfer = this->getDefaultComputeQueue();

				const VkPhysicalDeviceMemoryProperties &memProp =
					this->getVKDevice()->getPhysicalDevices()[0]->getMemoryProperties();

				VkCommandPool commandPool = this->getVKDevice()->createCommandPool(this->getDefaultComputeQueueIndex());
				std::vector<VkCommandBuffer> cmds =
					device->allocateCommandBuffers(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

				while (true) {
				}

				VkCommandBuffer cmd = cmds[0];

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				/*	Sync */

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->computeWilsomPrimePipeline);

				// vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->computePipelineLayout, 0, 1,
				//						&descriptorSets[i], 0, nullptr);

				const float localInvokation = 128; // TODO fetch

				// vkCmdPushConstants(cmd, this->computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
				//				   sizeof(glm::mat4x4), &mvp.model);

				vkCmdDispatch(cmd, std::ceil(primeCandidate / localInvokation), 1, 1);

				// vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
				// nullptr, 					 0, nullptr, blitBarriers.size(), blitBarriers.data());

				VKS_VALIDATE(vkEndCommandBuffer(cmd));

			} catch (const std::exception &ex) {
				std::cerr << "Failed During Benchmark" << std::endl;
				std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
			}
		}
	};

	class PrimeVKSample : public VKSample<Prime> {
	  public:
		PrimeVKSample() : VKSample<Prime>() {}
		 void customOptions(cxxopts::OptionAdder &options) override {
			options("Q,queue-index", "Select Queue to perform the memory bencharmk",
					cxxopts::value<int>()->default_value("-1"))("prime", "Prime Value",
																cxxopts::value<int>()->default_value("1000"));
		}

		std::vector<VkDeviceQueueCreateInfo>
		OnSelectQueue(const std::vector<std::shared_ptr<PhysicalDevice>> &physical_devices) override {
			std::vector<VkDeviceQueueCreateInfo> queues;

			/*	Select queue with transfer and the best timestamp resolution.	*/
			uint32_t timestampvalid = 0;
			int queueIndex = -1;
			for (size_t j = 0; j < physical_devices[0]->getQueueFamilyProperties().size(); j++) {
				/*  */
				const VkQueueFamilyProperties &familyProp = physical_devices[0]->getQueueFamilyProperties()[j];
				if ((familyProp.queueFlags & VK_QUEUE_COMPUTE_BIT) && familyProp.timestampValidBits > timestampvalid) {
					timestampvalid = familyProp.timestampValidBits;
					queueIndex = j;
				}
			}

			// TODO: fix reference.
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
		vksample::PrimeVKSample memoryTransfer;
		memoryTransfer.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}