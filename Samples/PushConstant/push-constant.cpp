#include "FPSCounter.h"
#include "Util/Time.hpp"
#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

class PushConstant : public VKWindow {
  private:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;

	std::vector<VkDescriptorSet> descriptorSets;

	VkDeviceSize uniformBufferSize;
	VkBuffer uniformBuffer = VK_NULL_HANDLE;
	VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
	std::vector<void *> mapMemory;

	struct UniformBufferBlock {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	} mvp;

	typedef struct _vertex_t {
		float pos[3];
		float uv[2];
	} Vertex;

  public:
	PushConstant(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->setTitle("Push Constant");
		this->show();
	}
	virtual ~PushConstant() {}

	virtual void release() override {

		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
		vkFreeMemory(getDevice(), vertexMemory, nullptr);

		vkUnmapMemory(getDevice(), uniformBufferMemory);
		vkDestroyBuffer(getDevice(), uniformBuffer, nullptr);
		vkFreeMemory(getDevice(), uniformBufferMemory, nullptr);

		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
	}

	const std::vector<Vertex> vertices = {{-1.0f, -1.0f, -1.0f, 0, 0}, // triangle 1 : begin
										  {-1.0f, -1.0f, 1.0f, 0, 1},
										  {-1.0f, 1.0f, 1.0f, 1, 1}, // triangle 1 : end
										  {1.0f, 1.0f, -1.0f, 0, 0}, // triangle 2 : begin
										  {-1.0f, -1.0f, -1.0f, 1, 1},
										  {-1.0f, 1.0f, -1.0f, 1, 0}, // triangle 2 : end
										  {1.0f, -1.0f, 1.0f, 0, 0},
										  {-1.0f, -1.0f, -1.0f, 0, 1},
										  {1.0f, -1.0f, -1.0f, 1, 1},
										  {1.0f, 1.0f, -1.0f, 0, 0},
										  {1.0f, -1.0f, -1.0f, 1, 1},
										  {-1.0f, -1.0f, -1.0f, 1, 0},
										  {-1.0f, -1.0f, -1.0f, 0, 0},
										  {-1.0f, 1.0f, 1.0f, 0, 1},
										  {-1.0f, 1.0f, -1.0f, 1, 1},
										  {1.0f, -1.0f, 1.0f, 0, 0},
										  {-1.0f, -1.0f, 1.0f, 1, 1},
										  {-1.0f, -1.0f, -1.0f, 0, 1},
										  {-1.0f, 1.0f, 1.0f, 0, 0},
										  {-1.0f, -1.0f, 1.0f, 0, 1},
										  {1.0f, -1.0f, 1.0f, 1, 1},
										  {1.0f, 1.0f, 1.0f, 0, 0},
										  {1.0f, -1.0f, -1.0f, 1, 1},
										  {1.0f, 1.0f, -1.0f, 1, 0},
										  {1.0f, -1.0f, -1.0f, 0, 0},
										  {1.0f, 1.0f, 1.0f, 0, 1},
										  {1.0f, -1.0f, 1.0f, 1, 1},
										  {1.0f, 1.0f, 1.0f, 0, 0},
										  {1.0f, 1.0f, -1.0f, 1, 1},
										  {-1.0f, 1.0f, -1.0f, 0, 1},
										  {1.0f, 1.0f, 1.0f, 0, 0},
										  {-1.0f, 1.0f, -1.0f, 0, 1},
										  {-1.0f, 1.0f, 1.0f, 1, 1},
										  {1.0f, 1.0f, 1.0f, 0, 0},
										  {-1.0f, 1.0f, 1.0f, 1, 1},
										  {1.0f, -1.0f, 1.0f, 1, 0}

	};

	VkPipeline createGraphicPipeline() {

		auto vertShaderCode = IOUtil::readFile("shaders/triangle-mvp-pushconstant.vert.spv");
		auto fragShaderCode = IOUtil::readFile("shaders/triangle-mvp.frag.spv");

		VkShaderModule vertShaderModule = VKHelper::createShaderModule(getDevice(), vertShaderCode);
		VkShaderModule fragShaderModule = VKHelper::createShaderModule(getDevice(), fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = 12;

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)width();
		viewport.height = (float)height();
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent.width = width();
		scissor.extent.height = height();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		/*	*/
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		VKS_VALIDATE(vkCreateDescriptorSetLayout(getDevice(), &layoutInfo, nullptr, &descriptorSetLayout));

		VkPushConstantRange pushRange = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4x4)};

		VKHelper::createPipelineLayout(getDevice(), pipelineLayout, {descriptorSetLayout}, {pushRange});

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = getDefaultRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VKS_VALIDATE(
			vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

		vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

		return graphicsPipeline;
	}

	virtual void Initialize() override {

		// TODO align
		uniformBufferSize = sizeof(UniformBufferBlock);
		uniformBufferSize += uniformBufferSize %
							 getVKDevice()->getPhysicalDevices()[0]->getDeviceLimits().minUniformBufferOffsetAlignment;

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

		VKHelper::createBuffer(getDevice(), uniformBufferSize * getSwapChainImageCount(), memProperties,
							   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							   uniformBuffer, uniformBufferMemory);

		for (size_t i = 0; i < getSwapChainImageCount(); i++) {
			void *_data;
			VKS_VALIDATE(
				vkMapMemory(getDevice(), uniformBufferMemory, uniformBufferSize * i, uniformBufferSize, 0, &_data));
			mapMemory.push_back(_data);
		}

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(getSwapChainImageCount());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(getSwapChainImageCount());

		vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descpool);

		/*	Create pipeline.	*/
		graphicsPipeline = createGraphicPipeline();

		std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocdescInfo{};
		allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocdescInfo.descriptorPool = descpool;
		allocdescInfo.descriptorSetCount = static_cast<uint32_t>(getSwapChainImageCount());
		allocdescInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(getSwapChainImageCount());
		VKS_VALIDATE(vkAllocateDescriptorSets(getDevice(), &allocdescInfo, descriptorSets.data()));

		for (size_t i = 0; i < getSwapChainImageCount(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffer;
			bufferInfo.offset = uniformBufferSize * i;
			bufferInfo.range = uniformBufferSize;

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(getDevice(), 1, &descriptorWrite, 0, nullptr);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VKS_VALIDATE(vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &vertexBuffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(getDevice(), vertexBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =
			VKHelper::findMemoryType(physicalDevice(), memRequirements.memoryTypeBits,
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				.value();

		VKS_VALIDATE(vkAllocateMemory(getDevice(), &allocInfo, nullptr, &vertexMemory));

		VKS_VALIDATE(vkBindBufferMemory(getDevice(), vertexBuffer, vertexMemory, 0));

		void *data;
		VKS_VALIDATE(vkMapMemory(getDevice(), vertexMemory, 0, bufferInfo.size, 0, &data));
		memcpy(data, vertices.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(getDevice(), vertexMemory);

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
		this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 100.0f);
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));
	}

	virtual void draw() override {

		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));
		this->mvp.model =
			glm::rotate(this->mvp.model, glm::radians(getTimer().getElapsed() * 45), glm::vec3(0.0f, 1.0f, 0.0f));
		this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));

		VkCommandBuffer cmd = getCommandBuffers(getCurrentFrameIndex());

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;

		VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = getDefaultRenderPass();
		renderPassInfo.framebuffer = getFrameBuffer(getCurrentFrameIndex());
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent.width = width();
		renderPassInfo.renderArea.extent.height = height();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdUpdateBuffer(cmd, uniformBuffer, uniformBufferSize * getCurrentFrameIndex(), sizeof(this->mvp),
						  &this->mvp);

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4x4), &mvp.model);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
								&descriptorSets[getCurrentFrameIndex()], 0, nullptr);

		vkCmdDraw(cmd, vertices.size(), 1, 0, 0);

		vkCmdEndRenderPass(cmd);

		VKS_VALIDATE(vkEndCommandBuffer(cmd));
	}
};

int main(int argc, const char **argv) {
	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, true},
																		   {"VK_KHR_xlib_surface", true}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, true},
																		 {VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, true}};

	try {
		VKSampleWindow<PushConstant> skybox(argc, argv, required_device_extensions, {}, required_instance_extensions);
		skybox.run();

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}