#include "FPSCounter.h"
#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VksCommon.h"
#include "common.hpp"
#include <SDL2/SDL.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

class SingleTextureWindow : public VKWindow {
  private:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
	VkDescriptorPool descpool;

	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void *> mapMemory;

	VkImage texture;
	VkImageView textureView;
	VkDeviceMemory textureMemory;
	long ntime;

	FPSCounter<float> fpsCounter;
	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	} mvp;

	typedef struct _vertex_t {
		float pos[3];
		float uv[2];
	} Vertex;

  public:
	SingleTextureWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		ntime = SDL_GetPerformanceCounter();
	}
	~SingleTextureWindow(void) {}

	virtual void Release(void) override {

		// vkFreeDescriptorSets
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroyImageView(getDevice(), textureView, nullptr);
		vkDestroyImage(getDevice(), texture, nullptr);
		vkFreeMemory(getDevice(), textureMemory, nullptr);

		vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
		vkFreeMemory(getDevice(), vertexMemory, nullptr);

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

		auto vertShaderCode = IOUtil::readFile("shaders/triangle-mvp.vert.spv");
		auto fragShaderCode = IOUtil::readFile("shaders/triangle-mvp-texture.frag.spv");

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

		/*	*/
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VKHelper::createDescriptorSetLayout(getDevice(), descriptorSetLayout, {uboLayoutBinding, samplerLayoutBinding});

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

		VKHelper::createPipelineLayout(getDevice(), pipelineLayout, {descriptorSetLayout});

		// VK_CHECK(vkCreatePipelineLayout(getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

		VkDynamicState dynamicStateEnables[1];
		dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.pNext = NULL;
		dynamicStateInfo.pDynamicStates = dynamicStateEnables;
		dynamicStateInfo.dynamicStateCount = 1;

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
		pipelineInfo.pDynamicState = &dynamicStateInfo;

		VK_CHECK(vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

		vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

		return graphicsPipeline;
	}

	virtual void Initialize(void) override {

		VkCommandBuffer cmd;
		std::vector<VkCommandBuffer> cmds = this->getLogicalDevice()->allocateCommandBuffers(
			getGraphicCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;

		VK_CHECK(vkBeginCommandBuffer(cmds[0], &beginInfo));

		ImageImporter::createImage("/home/voldie/test.png", getDevice(), getGraphicCommandPool(),
								   getDefaultGraphicQueue(), physicalDevice(), texture, textureMemory);

		vkEndCommandBuffer(cmds[0]);
		this->getLogicalDevice()->submitCommands(getDefaultGraphicQueue(), cmds);

		vkQueueWaitIdle(getDefaultGraphicQueue());
		vkFreeCommandBuffers(getDevice(), getGraphicCommandPool(), cmds.size(), cmds.data());

		textureView = VKHelper::createImageView(getDevice(), texture, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_B8G8R8A8_SRGB,
												VK_IMAGE_ASPECT_COLOR_BIT, 1);

		// VKHelper::createImageView();

		VkSampler sampler;
		VKHelper::createSampler(getDevice(), sampler);

		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(getSwapChainImageCount());
		uniformBuffersMemory.resize(getSwapChainImageCount());

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

		for (size_t i = 0; i < getSwapChainImageCount(); i++) {
			VKHelper::createBuffer(getDevice(), bufferSize, memProperties,
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								   uniformBuffers[i], uniformBuffersMemory[i]);
			void *_data;
			VK_CHECK(vkMapMemory(getDevice(), uniformBuffersMemory[i], 0, (size_t)sizeof(this->mvp), 0, &_data));
			mapMemory.push_back(_data);
		}

		/*	Allocate descriptor set.	*/
		const std::vector<VkDescriptorPoolSize> poolSize = {{
																VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
																static_cast<uint32_t>(getSwapChainImageCount()),
															},
															{
																VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
																static_cast<uint32_t>(getSwapChainImageCount()),
															}};

		descpool = VKHelper::createDescPool(getDevice(), poolSize, getSwapChainImageCount() * 2);

		// VkDescriptorPoolSize poolSize{};
		// poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// poolSize.descriptorCount = static_cast<uint32_t>(getSwapChainImageCount());

		// VkDescriptorPoolCreateInfo poolInfo{};
		// poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		// poolInfo.poolSizeCount = 1;
		// poolInfo.pPoolSizes = &poolSize;
		// poolInfo.maxSets = static_cast<uint32_t>(getSwapChainImageCount());

		// vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descpool);

		/*	Create pipeline.	*/
		graphicsPipeline = createGraphicPipeline();

		std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocdescInfo{};
		allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocdescInfo.descriptorPool = descpool;
		allocdescInfo.descriptorSetCount = static_cast<uint32_t>(getSwapChainImageCount());
		allocdescInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(getSwapChainImageCount());
		if (vkAllocateDescriptorSets(getDevice(), &allocdescInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < getSwapChainImageCount(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureView;
			imageInfo.sampler = sampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
								   0, nullptr);
		}

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
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				.value();

		VK_CHECK(vkAllocateMemory(getDevice(), &allocInfo, nullptr, &vertexMemory));

		VK_CHECK(vkBindBufferMemory(getDevice(), vertexBuffer, vertexMemory, 0));

		void *data;
		VK_CHECK(vkMapMemory(getDevice(), vertexMemory, 0, bufferInfo.size, 0, &data));
		memcpy(data, vertices.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(getDevice(), vertexMemory);

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		ntime = SDL_GetPerformanceCounter();

		VK_CHECK(vkQueueWaitIdle(getDefaultGraphicQueue()));
		this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 100.0f);
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));

		for (int i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

			/*	Transfer the new data 	*/
			// vkCmdTran

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = getDefaultRenderPass();
			renderPassInfo.framebuffer = getFrameBuffers()[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent.width = width;
			renderPassInfo.renderArea.extent.height = height;

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
			clearValues[1].depthStencil = {1.0f, 0};

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			// vkCmdUpdateBuffer(cmd, uniformBuffers[i], 0, sizeof(mvp), &mvp);

			VkBufferMemoryBarrier ub_barrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				.buffer = uniformBuffers[i],
				.offset = 0,
				.size = sizeof(mvp),
			};
			// ub_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			// ub_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;

			// vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL,
			// 1, 					 &ub_barrier, 0, NULL);
			VkViewport viewport = {
				.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			/*	*/
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0,
									nullptr);

			vkCmdDraw(cmd, vertices.size(), 1, 0, 0);

			vkCmdEndRenderPass(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw(void) override {

		float elapsedTime = ((float)(SDL_GetPerformanceCounter() - ntime) / (float)SDL_GetPerformanceFrequency());

		// fpsCounter.

		// printf("%f\n", elapsedTime);
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));
		this->mvp.model = glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45), glm::vec3(0.0f, 1.0f, 0.0f));
		this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));

		// Setup the range
		memcpy(mapMemory[getCurrentFrame()], &mvp, (size_t)sizeof(this->mvp));
		VkMappedMemoryRange stagingRange = {.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
											.memory = uniformBuffersMemory[getCurrentFrame()],
											.offset = 0,
											.size = (size_t)sizeof(this->mvp)};
		vkFlushMappedMemoryRanges(getDevice(), 1, &stagingRange);
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(argc, argv);
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		printf("%s\n", devices[0]->getDeviceName());
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);
		SingleTextureWindow window(core, d);

		window.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}