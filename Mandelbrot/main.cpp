#include "FPSCounter.h"
#include "VksCommon.h"
#include <VKWindow.h>
#include <glm/glm.hpp>

class MandelBrotWindow : public VKWindow {
  private:
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout graphicPipelineLayout = VK_NULL_HANDLE;
	VkPipeline computePipeline = VK_NULL_HANDLE;
	VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

	std::vector<VkImageView> computeImageViews;

	std::vector<VkDeviceMemory> paramMemory;
	VkBuffer paramBuffer;

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets;
	VkCommandPool computeCmdPool;
	std::vector<VkCommandBuffer> computeCmds;
	struct mandelbrot_param_t {
		float posX, posY;
		float mousePosX, mousePosY;
		float zoom; /*  */
		float c;	/*  */
		int nrSamples;
	} params = {};

	FPSCounter<float> fpsCounter;
	unsigned int paramMemSize = sizeof(params);

  public:
	MandelBrotWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->setTitle(std::string("MandelBrot"));
		//	fpsCounter = FPSCounter(100);
	}
	~MandelBrotWindow(void) {}

	virtual void Release(void) override {
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);
		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);

		for (unsigned int i = 0; i < computeImageViews.size(); i++)
			vkDestroyImageView(getDevice(), computeImageViews[i], nullptr);

		for (unsigned int i = 0; i < paramMemory.size(); i++)
			vkFreeMemory(getDevice(), paramMemory[i], nullptr);

		vkDestroyBuffer(getDevice(), paramBuffer, nullptr);
		vkDestroyPipeline(getDevice(), computePipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), computePipelineLayout, nullptr);
	}

	VkPipeline createComputePipeline(VkPipelineLayout *layout) {
		VkPipeline pipeline;

		auto compShaderCode = IOUtil::readFile("shaders/mandelbrot.comp.spv");

		VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = compShaderModule;
		compShaderStageInfo.pName = "main";

		std::array<VkDescriptorSetLayoutBinding, 2> uboLayoutBindings;
		/*	*/
		uboLayoutBindings[0].binding = 0;
		uboLayoutBindings[0].descriptorCount = 1;
		uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		uboLayoutBindings[0].pImmutableSamplers = nullptr;
		uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[1].binding = 1;
		uboLayoutBindings[1].descriptorCount = 1;
		uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBindings[1].pImmutableSamplers = nullptr;
		uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		/*	*/
		VKHelper::createDescriptorSetLayout(getDevice(), descriptorSetLayout, uboLayoutBindings);

		/*	*/
		VKHelper::createPipelineLayout(getDevice(), *layout, {descriptorSetLayout});

		pipeline = VKHelper::createComputePipeline(getDevice(), *layout, compShaderStageInfo);

		vkDestroyShaderModule(getDevice(), compShaderModule, nullptr);

		return pipeline;
	}

	virtual void Initialize(void) override {

		paramMemSize =
			std::max((unsigned int)getLogicalDevice()->getPhysicalDevices()[0]->getDeviceLimits().minMemoryMapAlignment,
					 (unsigned int)paramMemSize);
		/*	Create pipeline.	*/
		computePipeline = createComputePipeline(&computePipelineLayout);

		VkDeviceSize bufferSize = paramMemSize * getSwapChainImageCount();

		paramMemory.resize(1);

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

		for (size_t i = 0; i < paramMemory.size(); i++) {
			VKHelper::createBuffer(getDevice(), bufferSize, memProperties,
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								   paramBuffer, paramMemory[i]);
		}

		/*	Allocate descriptor set.	*/
		const std::vector<VkDescriptorPoolSize> poolSize = {{
																VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
																static_cast<uint32_t>(getSwapChainImageCount()),
															},
															{
																VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
																static_cast<uint32_t>(getSwapChainImageCount()),
															}};

		descpool = VKHelper::createDescPool(getDevice(), poolSize, getSwapChainImageCount() * 2);

		onResize(width(), height());

		computeCmdPool = getLogicalDevice()->createCommandPool(getLogicalDevice()->getDefaultComputeQueueIndex());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

		/*	*/
		computeImageViews.resize(getSwapChainImageCount());
		for (unsigned int i = 0; i < computeImageViews.size(); i++) {
			if (computeImageViews[i] != nullptr)
				vkDestroyImageView(getDevice(), computeImageViews[i], nullptr);
			computeImageViews[i] =
				VKHelper::createImageView(getDevice(), getSwapChainImages()[i], VK_IMAGE_VIEW_TYPE_2D,
										  getDefaultImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		/*	*/
		VKS_VALIDATE(vkResetDescriptorPool(getDevice(), descpool, 0));

		std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = descpool; // pool to allocate from.
		descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(getSwapChainImageCount());
		descriptorSetAllocateInfo.pSetLayouts = layouts.data();

		// allocate descriptor set.
		descriptorSets.resize(getSwapChainImageCount());
		VKS_VALIDATE(vkAllocateDescriptorSets(getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

		for (unsigned int i = 0; i < descriptorSets.size(); i++) {
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = computeImageViews[i];
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = paramBuffer;
			bufferInfo.offset = paramMemSize * i;
			bufferInfo.range = paramMemSize;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;
			descriptorWrites[0].pBufferInfo = nullptr;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = nullptr;
			descriptorWrites[1].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		for (unsigned int i = 0; i < getSwapChainImageCount(); i++) {
			void *data;
			VKS_VALIDATE(vkMapMemory(getDevice(), paramMemory[0], paramMemSize * i, paramMemSize, 0, &data));
			memcpy(data, &params, paramMemSize);
			vkUnmapMemory(getDevice(), paramMemory[0]);
		}

		// TODO resolve for if compute queue is not part of graphic queue.
		for (unsigned int i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1,
									&descriptorSets[i], 0, NULL);

			const int localInvokation = 16;

			vkCmdDispatch(cmd, std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageMemoryBarrier.image = getSwapChainImages()[i];
			imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
								 nullptr, 0, nullptr, 1, &imageMemoryBarrier);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw(void) override {
		// Setup the range
		void *data;
		VKS_VALIDATE(
			vkMapMemory(getDevice(), paramMemory[0], paramMemSize * getCurrentFrame(), paramMemSize, 0, &data));
		memcpy(data, &params, paramMemSize);
		vkUnmapMemory(getDevice(), paramMemory[0]);

		int x, y;
		SDL_GetMouseState(&x, &y);
		params.mousePosX = x;
		params.mousePosY = y;
		params.posX = 0;
		params.posY = 0;
		params.zoom = 1.0f;
		params.nrSamples = 128;

		fpsCounter.incrementFPS(SDL_GetPerformanceCounter());
		printf("fps: %d\n", fpsCounter.getFPS());
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();

		std::shared_ptr<VKDevice> ldevice = std::make_shared<VKDevice>(devices, required_device_extensions);

		MandelBrotWindow window(core, ldevice);

		window.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}