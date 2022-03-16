#include "FPSCounter.h"
#include "VKSampleWindow.h"
#include "VksCommon.h"
#include <VKWindow.h>
#include <glm/glm.hpp>

class ComputeHeadless : public VKSampleSession {
  public:
	ComputeHeadless(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKSampleSession(core, device) {
		prepare_pipeline();
		cmdBuffer = this->device->allocateCommandBuffers(this->compute_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 2);
	}

	~ComputeHeadless() {

		vkDestroyDescriptorSetLayout(getDevice(), computeDescriptorSetLayout, nullptr);

		vkDestroyPipeline(getDevice(), computePipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), computePipelineLayout, nullptr);

		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);
	}

	void prepare_pipeline() {

		auto compShaderCode = IOUtil::readFile("shaders/reactiondiffusion.comp.spv");

		VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = compShaderModule;
		compShaderStageInfo.pName = "main";

		std::array<VkDescriptorSetLayoutBinding, 4> uboLayoutBindings;
		/*	*/
		uboLayoutBindings[0].binding = 0;
		uboLayoutBindings[0].descriptorCount = 1;
		uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uboLayoutBindings[0].pImmutableSamplers = nullptr;
		uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[1].binding = 1;
		uboLayoutBindings[1].descriptorCount = 1;
		uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uboLayoutBindings[1].pImmutableSamplers = nullptr;
		uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[2].binding = 2;
		uboLayoutBindings[2].descriptorCount = 1;
		uboLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		uboLayoutBindings[2].pImmutableSamplers = nullptr;
		uboLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[3].binding = 3;
		uboLayoutBindings[3].descriptorCount = 1;
		uboLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBindings[3].pImmutableSamplers = nullptr;
		uboLayoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VKHelper::createDescriptorSetLayout(getDevice(), computeDescriptorSetLayout, uboLayoutBindings);

		VKHelper::createPipelineLayout(getDevice(), computePipelineLayout, {computeDescriptorSetLayout});

		VkComputePipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stage = compShaderStageInfo;
		pipelineCreateInfo.layout = computePipelineLayout;

		VKS_VALIDATE(
			vkCreateComputePipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &computePipeline));
		vkDestroyShaderModule(getDevice(), compShaderModule, nullptr);
	}

	virtual void run() override {
		VkCommandBuffer cmd = cmdBuffer[0];

		while (true) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1,
									&descriptorSets[0], 0, nullptr);

			VKHelper::memoryBarrier(cmd, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
									VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

			vkCmdDispatch(cmd, 512, 512, 1);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));

			this->device->submitCommands(computeQueue, {cmd});
			VKS_VALIDATE(vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
		}
	}

  private:
	const int localInvokation = 16;
	VkQueue computeQueue = VK_NULL_HANDLE;
	VkPipeline computePipeline = VK_NULL_HANDLE;
	VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> cmdBuffer;
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, false},
																		   {"VK_KHR_xlib_surface", false}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, false}};

	try {
		VKSampleWindow<ComputeHeadless> computeheadless(argc, argv, required_device_extensions, {},
														required_instance_extensions);
		computeheadless.run();

	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}