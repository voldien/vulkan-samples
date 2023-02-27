#include "Importer/ImageImport.h"
#include "Util/Time.hpp"
#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <Util/CameraController.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

class BasicTessellation : public VKWindow {
  private:
	struct {
		VkPipeline basic_tessellation;
		VkPipeline wireframe_tessellation;
	} pipelines = {VK_NULL_HANDLE};
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	/*	*/
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;

	/*	*/
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkBuffer indiceBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
	size_t nrElements;

	/*	*/
	std::vector<VkDescriptorSet> descriptorSets;

	/*	*/
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	std::vector<void *> mapMemory;
	VkDeviceSize uniformBufferSize;

	VkSampler sampler = VK_NULL_HANDLE;

	/*	Height map texture.	*/
	VkImage heightTexture = VK_NULL_HANDLE;
	VkImageView heightTextureView = VK_NULL_HANDLE;
	VkDeviceMemory heightTextureMemory = VK_NULL_HANDLE;

	/*	*/
	VkImage diffuseTexture = VK_NULL_HANDLE;
	VkImageView diffuseTextureView = VK_NULL_HANDLE;
	VkDeviceMemory diffuseTextureMemory = VK_NULL_HANDLE;
	CameraController cameraController;
	struct UniformBufferBlock {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 modelView;
		glm::mat4 modelViewProjection;
		glm::mat4 normalMatrix;

		/*	Tessellation settings.	*/
		glm::vec3 eyePos = glm::vec3(1.0f);
		float gDisplace = 1.0f;

		/*	Light sun.	*/
		glm::vec4 direction;
		glm::vec4 lightColor;
		float lightIntensity;
	} mvp;

	typedef struct _vertex_t {
		float pos[3];
		float uv[2];
	} Vertex;

	const std::string diffuseTexturePath = "asset/tessellation_diffusemap.png";
	const std::string heightTexturePath = "asset/tessellation_heightmap.png";
	/*	*/
	const std::string vertexShaderPath = "Shaders/tessellation/tessellation.vert";
	const std::string fragmentShaderPath = "Shaders/tessellation/tessellation.frag";
	const std::string ControlShaderPath = "Shaders/tessellation/tessellation.tesc";
	const std::string EvoluationShaderPath = "Shaders/tessellation/tessellation.tese";

  public:
	BasicTessellation(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {

		VkPhysicalDeviceFeatures enabledFeatures;
		VkPhysicalDeviceFeatures deviceFeatures = this->getPhysicalDevice()->getFeatures();

		if (!deviceFeatures.tessellationShader) {
		}
		// Example uses tessellation shaders
		if (deviceFeatures.tessellationShader) {
			enabledFeatures.tessellationShader = VK_TRUE;
		} else {
		}

		// Fill mode non solid is required for wireframe display
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		}

		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}

		this->setTitle("Basic Tessellation");
		this->show();
	}
	virtual ~BasicTessellation() {}

	virtual void release() override {

		VKS_VALIDATE(vkFreeDescriptorSets(getDevice(), descpool, descriptorSets.size(), descriptorSets.data()));
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroySampler(getDevice(), sampler, nullptr);

		vkDestroyImageView(getDevice(), heightTextureView, nullptr);
		vkDestroyImage(getDevice(), heightTexture, nullptr);
		vkFreeMemory(getDevice(), heightTextureMemory, nullptr);

		vkDestroyImageView(getDevice(), diffuseTextureView, nullptr);
		vkDestroyImage(getDevice(), diffuseTexture, nullptr);
		vkFreeMemory(getDevice(), diffuseTextureMemory, nullptr);

		vkDestroyBuffer(getDevice(), uniformBuffer, nullptr);
		vkUnmapMemory(getDevice(), uniformBufferMemory);
		vkFreeMemory(getDevice(), uniformBufferMemory, nullptr);

		vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
		vkFreeMemory(getDevice(), vertexMemory, nullptr);

		vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), pipelines.basic_tessellation, nullptr);
		vkDestroyPipeline(getDevice(), pipelines.wireframe_tessellation, nullptr);
		vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
	}

	VkPipeline createGraphicPipeline() {

		auto vertShaderCode = vksample::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, fragcore::FileSystem::getFileSystem());
		auto fragShaderCode = vksample::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, fragcore::FileSystem::getFileSystem());
		auto teseShaderCode = vksample::IOUtil::readFileData<uint32_t>(this->EvoluationShaderPath, fragcore::FileSystem::getFileSystem());
		auto tescShaderCode = vksample::IOUtil::readFileData<uint32_t>(this->ControlShaderPath, fragcore::FileSystem::getFileSystem());

		VkShaderModule vertShaderModule = VKHelper::createShaderModule(getDevice(), vertShaderCode);
		VkShaderModule fragShaderModule = VKHelper::createShaderModule(getDevice(), fragShaderCode);
		VkShaderModule teseShaderModule = VKHelper::createShaderModule(getDevice(), teseShaderCode);
		VkShaderModule tescShaderModule = VKHelper::createShaderModule(getDevice(), tescShaderCode);

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

		VkPipelineShaderStageCreateInfo teseShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		fragShaderStageInfo.module = teseShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo tescShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		fragShaderStageInfo.module = tescShaderModule;
		fragShaderStageInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo,
																	 teseShaderStageInfo, tescShaderStageInfo};

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
		depthStencil.depthTestEnable = VK_TRUE;
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

		VKS_VALIDATE(vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
											   &pipelines.basic_tessellation));

		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
		VKS_VALIDATE(vkCreateGraphicsPipelines(getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
											   &pipelines.wireframe_tessellation));

		vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);
		vkDestroyShaderModule(getDevice(), teseShaderModule, nullptr);
		vkDestroyShaderModule(getDevice(), tescShaderModule, nullptr);

		return nullptr;
	}

	virtual void Initialize() override {

		VkCommandBuffer cmd;
		std::vector<VkCommandBuffer> cmds =
			this->getVKDevice()->allocateCommandBuffers(getGraphicCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;

		VKS_VALIDATE(vkBeginCommandBuffer(cmds[0], &beginInfo));

		vkEndCommandBuffer(cmds[0]);
		this->getVKDevice()->submitCommands(getDefaultGraphicQueue(), cmds);

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
		vkFreeCommandBuffers(getDevice(), getGraphicCommandPool(), cmds.size(), cmds.data());

		/*	Diffuse Texture.	*/
		vksample::ImageImporter imageImporter(fragcore::FileSystem::getFileSystem(), *this->getVKDevice());
		imageImporter.createImage2D(this->diffuseTexturePath.c_str(), getDevice(), getGraphicCommandPool(),
									getDefaultGraphicQueue(), physicalDevice(), heightTexture, heightTextureMemory);

		/*	Create random noise texture.	*/
		heightTextureView = VKHelper::createImageView(getDevice(), heightTexture, VK_IMAGE_VIEW_TYPE_2D,
													  VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		VKHelper::createSampler(getDevice(), sampler);

		/*	Compute uniform buffer size, in respect to the alignment requirement.	*/
		uniformBufferSize = sizeof(UniformBufferBlock);
		size_t minMapBufferSize =
			getVKDevice()->getPhysicalDevices()[0]->getDeviceLimits().minUniformBufferOffsetAlignment;
		uniformBufferSize += minMapBufferSize - (uniformBufferSize % minMapBufferSize);

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

		/*	Create pipeline.	*/
		createGraphicPipeline();

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
			bufferInfo.offset = i * uniformBufferSize;
			bufferInfo.range = uniformBufferSize;

			VkDescriptorImageInfo imageDiffuseInfo{};
			imageDiffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageDiffuseInfo.imageView = diffuseTextureView;
			imageDiffuseInfo.sampler = sampler;

			VkDescriptorImageInfo imageHeightInfo{};
			imageHeightInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageHeightInfo.imageView = heightTextureView;
			imageHeightInfo.sampler = sampler;

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
			descriptorWrites[1].pImageInfo = &imageDiffuseInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 1;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &imageHeightInfo;

			vkUpdateDescriptorSets(getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
								   0, nullptr);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(float) * 1; // vertices.size();//TODO fix
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
		// memcpy(data, vertices.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(getDevice(), vertexMemory);

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
		this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 100.0f);
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));

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

			vkCmdSetLineWidth(cmd, 1.0f);

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			/*	*/
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.basic_tessellation);

			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0,
									nullptr);

			vkCmdDrawIndexed(cmd, this->nrElements, 1, 0, 0, 1);

			vkCmdEndRenderPass(cmd);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw() override {

		this->cameraController.update(getTimer().deltaTime());
		float elapsedTime = getTimer().getElapsed();
		glm::mat4 viewMatrix = this->cameraController.getViewMatrix();

		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = viewMatrix;
		this->mvp.modelViewProjection = this->mvp.model * this->mvp.view * this->mvp.proj;
		this->mvp.model = glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45), glm::vec3(0.0f, 1.0f, 0.0f));
		this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));

		// Setup the range
		memcpy(mapMemory[getCurrentFrameIndex()], &mvp, (size_t)sizeof(this->mvp));
	}

	virtual void update() {}
};

// TODO add texture option.

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		VKSample<BasicTessellation> tessellation;
		tessellation.run(argc, argv, required_device_extensions, {},
													   required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}