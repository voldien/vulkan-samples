#include "Importer/ImageImport.h"
#include "Util/Time.hpp"

#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <Util/CameraController.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

class SkyboxCubeMap : public VKWindow {
  private:
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;

	VkSampler sampler = VK_NULL_HANDLE;
	VkImage texture = VK_NULL_HANDLE;
	VkImageView skyboxTextureView = VK_NULL_HANDLE;
	VkDeviceMemory textureMemory = VK_NULL_HANDLE;

	std::vector<VkDescriptorSet> descriptorSets;
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	std::vector<void *> mapMemory;

	CameraController cameraController;

	struct UniformBufferBlock {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 modelView;
		glm::mat4 modelViewProjection;
		glm::vec4 diffuseColor;
	} mvp;

	typedef struct _vertex_t {
		float pos[3];
		float uv[2];
	} Vertex;

  public:
	SkyboxCubeMap(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->setTitle("Skybox Cubemap");
		this->show();
	}
	virtual ~SkyboxCubeMap() {}

	virtual void release() override {

		vkDestroySampler(getDevice(), sampler, nullptr);

		vkDestroyImageView(getDevice(), skyboxTextureView, nullptr);
		vkDestroyImage(getDevice(), texture, nullptr);
		vkFreeMemory(getDevice(), textureMemory, nullptr);

		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
		vkFreeMemory(getDevice(), vertexMemory, nullptr);

		vkDestroyBuffer(getDevice(), uniformBuffer, nullptr);
		vkUnmapMemory(getDevice(), uniformBufferMemory);
		vkFreeMemory(getDevice(), uniformBufferMemory, nullptr);

		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
	}

	const std::vector<Vertex> vertices = {{-1.0f, -1.0f, -1.0f, 0, 0}, // triangle 1 : begin
										  {-1.0f, -1.0f, 1.0f, 0, 1},
										  {-1.0f, 1.0f, 1.0f, 1, 1}, // triangle 1 : end
										  {1.0f, 1.0f, -1.0f, 1, 1}, // triangle 2 : begin
										  {-1.0f, -1.0f, -1.0f, 1, 0},
										  {-1.0f, 1.0f, -1.0f, 0, 0}, // triangle 2 : end
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
	const std::string vertexShaderPath = "shaders/skybox/skybox.vert.spv";
	const std::string fragmentShaderPath = "shaders/skybox/cubemap.frag.spv";

	VkPipeline createGraphicPipeline() {
		auto vertShaderCode =
			vksample::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, fragcore::FileSystem::getFileSystem());
		auto fragShaderCode =
			vksample::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, fragcore::FileSystem::getFileSystem());

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

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions;

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

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
		viewport.width = static_cast<float>(this->width());
		viewport.height = static_cast<float>(this->height());
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
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VKHelper::createPipelineLayout(getDevice(), pipelineLayout, {descriptorSetLayout});

		VkDynamicState dynamicStateEnables[1];
		dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.pNext = NULL;
		dynamicStateInfo.pDynamicStates = dynamicStateEnables;
		dynamicStateInfo.dynamicStateCount = 1;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
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

		VKS_VALIDATE(
			vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

		vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

		return graphicsPipeline;
	}

	virtual void Initialize() override {
		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

		/*	*/
		vksample::ImageImporter imageImporter(fragcore::FileSystem::getFileSystem(), *this->getVKDevice());
		imageImporter.createImage2D("asset/panorama.png", this->getDevice(), getGraphicCommandPool(),
									getDefaultGraphicQueue(), physicalDevice(), texture, textureMemory);

		skyboxTextureView = VKHelper::createImageView(getDevice(), texture, VK_IMAGE_VIEW_TYPE_2D,
													  VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VKHelper::createSampler(getDevice(), sampler);

		/*	Allocate uniform buffers.	*/
		VkDeviceSize uniformBufferSize = sizeof(UniformBufferBlock);
		// TODO align
		// bufferSize += bufferSize %

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

		VKHelper::createBuffer(getDevice(), uniformBufferSize * getSwapChainImageCount(), memProperties,
							   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							   uniformBuffer, uniformBufferMemory);

		for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {
			void *_data;
			VKS_VALIDATE(vkMapMemory(getDevice(), uniformBufferMemory, uniformBufferSize * i, (size_t)sizeof(this->mvp),
									 0, &_data));
			mapMemory.push_back(_data);
		}

		/*	Create pipeline.	*/
		graphicsPipeline = createGraphicPipeline();

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

		/*	*/
		std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocdescInfo{};
		allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocdescInfo.descriptorPool = descpool;
		allocdescInfo.descriptorSetCount = static_cast<uint32_t>(getSwapChainImageCount());
		allocdescInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(getSwapChainImageCount());
		VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descriptorSets.data()));

		for (size_t i = 0; i < getSwapChainImageCount(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffer;
			bufferInfo.offset = uniformBufferSize * i;
			bufferInfo.range = uniformBufferSize;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = skyboxTextureView;
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

			vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
								   descriptorWrites.data(), 0, nullptr);
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

		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
		this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 100.0f);

		/*	Create command buffers.	*/
		for (size_t i = 0; i < this->getNrCommandBuffers(); i++) {
			VkCommandBuffer cmd = this->getCommandBuffers(i);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = getDefaultRenderPass();
			renderPassInfo.framebuffer = getFrameBuffer(i);
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent.width = width;
			renderPassInfo.renderArea.extent.height = height;

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
			clearValues[1].depthStencil = {1.0f, 0};

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

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

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw() override {

		this->cameraController.update(getTimer().deltaTime());
		glm::mat4 viewMatrix = this->cameraController.getViewMatrix();
		// TODO add character controller.
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = viewMatrix;
		this->mvp.modelViewProjection = this->mvp.model * this->mvp.proj * this->mvp.view;

		// Setup the range
		memcpy(mapMemory[getCurrentFrameIndex()], &mvp, (size_t)sizeof(this->mvp));
		// 	VkMappedMemoryRange stagingRange{};
		// 	stagingRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		// 	stagingRange.memory = uniformBuffersMemory[getCurrentFrameIndex()];
		// 	stagingRange.offset = 0;
		// 	stagingRange.size = (size_t)sizeof(this->mvp);
		// 	vkFlushMappedMemoryRanges(getDevice(), 1, &stagingRange);
	}

	virtual void update() {}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};
	// TODO add custom argument options for adding path of the texture and what type.
	try {
		VKSample<SkyboxCubeMap> skybox;
		skybox.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}