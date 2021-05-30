#include "VksCommon.h"
#include "VKHelper.h"
#include "common.hpp"
#include <SDL2/SDL.h>
#include <VKWindow.h>
#include <glm/glm.hpp>

class MandelBrotWindow : public VKWindow {
  private:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout graphicPipelineLayout = VK_NULL_HANDLE;
	VkPipeline computePipeline = VK_NULL_HANDLE;
	VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool;

	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void *> mapMemory;

  public:
	MandelBrotWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		//	this->setTitle(std::string("Triangle"));
	}
	~MandelBrotWindow(void) {}

	virtual void Release(void) override {
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
		vkFreeMemory(getDevice(), vertexMemory, nullptr);

		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), graphicPipelineLayout, nullptr);

		vkDestroyPipeline(getDevice(), computePipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), computePipelineLayout, nullptr);
	}

	const std::vector<float> vertices = {0.0f, -0.5f, 1.0f,	 1.0f, 1.0f, 0.5f, 0.5f, 0.0f,
										 1.0f, 0.0f,  -0.5f, 0.5f, 0.0f, 0.0f, 1.0f};

	VkPipeline createGraphicPipeline(VkPipelineLayout *layout) { return VK_NULL_HANDLE; }

	void createComputePipeline(VkPipelineLayout *layout) {
		// auto compShaderCode = IOUtil::readFile("shaders/mandelbrot.comp.spv");

		// VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

		// VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		// compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		// compShaderStageInfo.module = compShaderModule;
		// compShaderStageInfo.pName = "main";

		// VkPipelineShaderStageCreateInfo shaderStages[] = {compShaderStageInfo};

		// VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		// vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// VkVertexInputBindingDescription bindingDescription = {};
		// bindingDescription.binding = 0;
		// bindingDescription.stride = 5 * 4;
		// bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

		// attributeDescriptions[0].binding = 0;
		// attributeDescriptions[0].location = 0;
		// attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		// attributeDescriptions[0].offset = 0;

		// attributeDescriptions[1].binding = 0;
		// attributeDescriptions[1].location = 1;
		// attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		// attributeDescriptions[1].offset = 8;

		// vertexInputInfo.vertexBindingDescriptionCount = 1;
		// vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		// vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		// vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// VkDescriptorSetLayoutBinding uboLayoutBinding{};
		// uboLayoutBinding.binding = 0;
		// uboLayoutBinding.descriptorCount = 1;
		// uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// uboLayoutBinding.pImmutableSamplers = nullptr;
		// uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// VkDescriptorSetLayoutCreateInfo layoutInfo{};
		// layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		// layoutInfo.bindingCount = 1;
		// layoutInfo.pBindings = &uboLayoutBinding;

		// VK_CHECK(vkCreateDescriptorSetLayout(getDevice(), &layoutInfo, nullptr, &descriptorSetLayout));

		// VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		// inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		// inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// inputAssembly.primitiveRestartEnable = VK_FALSE;

		// VkViewport viewport{};
		// viewport.x = 0.0f;
		// viewport.y = 0.0f;
		// viewport.width = (float)width();
		// viewport.height = (float)height();
		// viewport.minDepth = 0.0f;
		// viewport.maxDepth = 1.0f;

		// VkRect2D scissor{};
		// scissor.offset = {0, 0};
		// scissor.extent.width = width();
		// scissor.extent.height = height();

		// VkPipelineViewportStateCreateInfo viewportState{};
		// viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		// viewportState.viewportCount = 1;
		// viewportState.pViewports = &viewport;
		// viewportState.scissorCount = 1;
		// viewportState.pScissors = &scissor;

		// VkPipelineRasterizationStateCreateInfo rasterizer{};
		// rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// rasterizer.depthClampEnable = VK_FALSE;
		// rasterizer.rasterizerDiscardEnable = VK_FALSE;
		// rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// rasterizer.lineWidth = 1.0f;
		// rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		// rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		// rasterizer.depthBiasEnable = VK_FALSE;

		// VkPipelineMultisampleStateCreateInfo multisampling{};
		// multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		// multisampling.sampleShadingEnable = VK_FALSE;
		// multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		// colorBlendAttachment.colorWriteMask =
		// 	VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// colorBlendAttachment.blendEnable = VK_FALSE;

		// VkPipelineColorBlendStateCreateInfo colorBlending{};
		// colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		// colorBlending.logicOpEnable = VK_FALSE;
		// colorBlending.logicOp = VK_LOGIC_OP_COPY;
		// colorBlending.attachmentCount = 1;
		// colorBlending.pAttachments = &colorBlendAttachment;
		// colorBlending.blendConstants[0] = 0.0f;
		// colorBlending.blendConstants[1] = 0.0f;
		// colorBlending.blendConstants[2] = 0.0f;
		// colorBlending.blendConstants[3] = 0.0f;

		// VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		// pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// pipelineLayoutInfo.setLayoutCount = 1;
		// pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		// pipelineLayoutInfo.pushConstantRangeCount = 0;

		// VK_CHECK(vkCreatePipelineLayout(getDevice(), &pipelineLayoutInfo, nullptr, &graphicPipelineLayout));

		// VkGraphicsPipelineCreateInfo pipelineInfo{};
		// pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// pipelineInfo.stageCount = 2;
		// pipelineInfo.pStages = shaderStages;
		// pipelineInfo.pVertexInputState = &vertexInputInfo;
		// pipelineInfo.pInputAssemblyState = &inputAssembly;
		// pipelineInfo.pViewportState = &viewportState;
		// pipelineInfo.pRasterizationState = &rasterizer;
		// pipelineInfo.pMultisampleState = &multisampling;
		// pipelineInfo.pColorBlendState = &colorBlending;
		// pipelineInfo.layout = graphicPipelineLayout;
		// pipelineInfo.renderPass = getDefaultRenderPass();
		// pipelineInfo.subpass = 0;
		// pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		// VK_CHECK(vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));


		// vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
		// vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

		// return graphicsPipeline;
	}

	virtual void Initialize(void) override {
		/*	Create pipeline.	*/
		//computePipeline = createComputePipeline(&computePipelineLayout);
		graphicsPipeline = createGraphicPipeline(&graphicPipelineLayout);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &vertexBuffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(getDevice(), vertexBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =
			VKHelper::findMemoryType(physicalDevice(), memRequirements.memoryTypeBits,
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT).value();

		VK_CHECK(vkAllocateMemory(getDevice(), &allocInfo, nullptr, &vertexMemory));

		VK_CHECK(vkBindBufferMemory(getDevice(), vertexBuffer, vertexMemory, 0));

		void *data;
		VK_CHECK(vkMapMemory(getDevice(), vertexMemory, 0, bufferInfo.size, 0, &data));
		memcpy(data, vertices.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(getDevice(), vertexMemory);

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VK_CHECK(vkQueueWaitIdle(getDefaultGraphicQueue()));

		for (int i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = getDefaultRenderPass();
			renderPassInfo.framebuffer = getFrameBuffers()[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent.width = width;
			renderPassInfo.renderArea.extent.height = height;

			VkClearValue clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
			/*	*/
			//vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

			vkCmdDraw(cmd, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));
		}
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {

	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(argc, argv);
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		printf("%s\n", devices[0]->getDeviceName());
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);

		MandelBrotWindow window(core,d );

		window.run();
	} catch (std::exception &ex) {
		// std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}