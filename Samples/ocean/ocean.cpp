#include "FPSCounter.h"
#include "Util/CameraController.h"
#include "Util/Time.hpp"
#include "VksCommon.h"
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

class Ocean : public VKWindow {
  private:
	const std::string vertexShader = "ocean/ocean.vert";
	const std::string fragmentShader = "ocean/ocean.frag";
	const std::string particleCompute = "ocean/ocean.comp";

	struct UniformBufferBlock {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 modelView;
		alignas(16) glm::mat4 modelViewProjection;
		glm::vec4 diffuseColor;
		float delta;

		/*	*/
		float speed;

	} mvp;

	struct ParticleSimulatorVariables {
	} particleSimUniforms;

	typedef struct particle_t {
		glm::vec3 position; /*	Position.	*/
		glm::vec4 velocity; /*	Velocity.	*/
		float t;			/*	Time.	*/
	} Particle;

	/*	Compute pipeline.	*/
	VkPipeline particleSim = VK_NULL_HANDLE;
	VkPipelineLayout particleSimLayout = VK_NULL_HANDLE;

	/*	Graphic pipelines.	*/
	VkPipeline particleGraphicPipeline = VK_NULL_HANDLE;
	VkPipelineLayout particleGraphicLayout = VK_NULL_HANDLE;

	/*	*/
	std::vector<VkBuffer> particleBuffers;
	std::vector<VkDeviceMemory> particleBufferMemory;

	/*	*/
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemories; // TODO make a single buffer
	std::vector<void *> mapMemory;

	/*	*/
	VkSampler sampler = VK_NULL_HANDLE;
	VkImage texture = VK_NULL_HANDLE;
	VkImageView textureView = VK_NULL_HANDLE;
	VkDeviceMemory textureMemory = VK_NULL_HANDLE;

	/*	*/
	VkBuffer vectorFieldBuffer;
	VkDeviceMemory vectorFieldMemory;

	/*	*/
	VkDescriptorSetLayout particleComputeDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout particleGraphicDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descpool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> particleComputeDescriptorSets;
	std::vector<VkDescriptorSet> particleGraphicDescriptorSets;

	VkDeviceSize UniformParamMemSize = sizeof(mvp);
	/*	*/
	CameraController cameraController;
	// TODO add ass configurable param.
	const int localInvokation = 32;
	const unsigned int nrParticles = localInvokation * 256;

  public:
	Ocean(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->setTitle("Ocean");
		this->show();
	}

	virtual ~Ocean() {}

	virtual void release() override {

		/*	*/
		vkDestroyDescriptorPool(getDevice(), descpool, nullptr);

		vkDestroySampler(getDevice(), sampler, nullptr);

		vkDestroyImageView(getDevice(), textureView, nullptr);
		vkDestroyImage(getDevice(), texture, nullptr);
		vkFreeMemory(getDevice(), textureMemory, nullptr);

		/*	*/
		for (unsigned int i = 0; i < particleBuffers.size(); i++) {
			vkDestroyBuffer(getDevice(), particleBuffers[i], nullptr);
			vkFreeMemory(getDevice(), particleBufferMemory[i], nullptr);
		}

		/*	*/
		for (unsigned int i = 0; i < uniformBuffers.size(); i++) {
			vkDestroyBuffer(getDevice(), uniformBuffers[i], nullptr);
			vkUnmapMemory(getDevice(), uniformBuffersMemories[i]);
			vkFreeMemory(getDevice(), uniformBuffersMemories[i], nullptr);
		}

		/*	*/
		vkDestroyDescriptorSetLayout(getDevice(), particleGraphicDescriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), particleGraphicPipeline, nullptr);
		vkDestroyPipelineLayout(getDevice(), particleGraphicLayout, nullptr);
		/*	*/
		vkDestroyDescriptorSetLayout(getDevice(), particleComputeDescriptorSetLayout, nullptr);
		vkDestroyPipeline(getDevice(), particleSim, nullptr);
		vkDestroyPipelineLayout(getDevice(), particleSimLayout, nullptr);
	}

	// void generateHeightField(cl_float2 *h0, unsigned int fftInputH, unsigned int fftInputW) {
	// 	float fMultiplier, fAmplitude, fTheta;
	// 	for (unsigned int y = 0; y < fftInputH; y++) {
	// 		for (unsigned int x = 0; x < fftInputW; x++) {
	// 			float kx = OPENCL_PI_F * x / (float)_patchSize;
	// 			float ky = 2.0f * OPENCL_PI_F * y / (float)_patchSize;
	// 			float Er = 2.0f * rand() / (float)RAND_MAX - 1.0f;
	// 			float Ei = 2.0f * rand() / (float)RAND_MAX - 1.0f;
	// 			if (!((kx == 0.f) && (ky == 0.f))) {
	// 				fMultiplier = sqrt(phillips(kx, ky, windSpeed, windDir));
	// 			} else {
	// 				fMultiplier = 0.f;
	// 			}
	// 			fAmplitude = RandNormal(0.0f, 1.0f);
	// 			fTheta = rand() / (float)RAND_MAX * 2 * OPENCL_PI_F;
	// 			float h0_re = fMultiplier * fAmplitude * Er;
	// 			float h0_im = fMultiplier * fAmplitude * Ei;
	// 			int i = y * fftInputW + x;
	// 			cl_float2 tmp = {h0_re, h0_im};
	// 			h0[i] = tmp;
	// 		}
	// 	}
	// }

	// float phillips(float kx, float ky, float windSpeed, float windDirection) {
	// 	float fWindDir = windDirection * OPENCL_PI_F / 180.0f;
	// 	static float A = 2.f * .00000005f;
	// 	float L = windSpeed * windSpeed / 9.81f;
	// 	float w = L / 75;
	// 	float ksqr = kx * kx + ky * ky;
	// 	float kdotwhat = kx * cosf(fWindDir) + ky * sinf(fWindDir);
	// 	kdotwhat = max(0.0f, kdotwhat);
	// 	float result =
	// 		(float)(A * (pow(2.7183f, -1.0f / (L * L * ksqr)) * (kdotwhat * kdotwhat)) / (ksqr * ksqr * ksqr));
	// 	float damp = (float)expf(-ksqr * w * w);
	// 	damp = expf(-1.0 / (ksqr * L * L));
	// 	result *= kdotwhat < 0.0f ? 0.25f : 1.0f;
	// 	return (result * damp);
	// }

	VkPipeline createGraphicPipeline(VkPipelineLayout *layout) {

		VkPipeline graphicsPipeline;

		auto vertShaderCode = IOUtil::readFile("shaders/particle.vert.spv");
		auto fragShaderCode = IOUtil::readFile("shaders/particle.frag.spv");

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
		bindingDescription.stride = sizeof(Particle);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
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
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VKHelper::createDescriptorSetLayout(getDevice(), particleGraphicDescriptorSetLayout,
											{uboLayoutBinding, samplerLayoutBinding});

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

		VKHelper::createPipelineLayout(getDevice(), *layout, {particleComputeDescriptorSetLayout});

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
		pipelineInfo.layout = *layout;
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

	VkPipeline createComputePipeline(VkPipelineLayout *layout) {
		VkPipeline pipeline;

		auto compShaderCode = IOUtil::readFile("shaders/particle.comp.spv");

		VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = compShaderModule;
		compShaderStageInfo.pName = "main";

		/*	Set descriptior layout of pipeline.	*/
		std::array<VkDescriptorSetLayoutBinding, 2> uboLayoutBindings;
		/*	*/
		uboLayoutBindings[0].binding = 0;
		uboLayoutBindings[0].descriptorCount = 1;
		uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uboLayoutBindings[0].pImmutableSamplers = nullptr;
		uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		uboLayoutBindings[1].binding = 1;
		uboLayoutBindings[1].descriptorCount = 1;
		uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBindings[1].pImmutableSamplers = nullptr;
		uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		/*	*/
		VKHelper::createDescriptorSetLayout(getDevice(), particleComputeDescriptorSetLayout, uboLayoutBindings);

		/*	*/
		VKHelper::createPipelineLayout(getDevice(), *layout, {particleComputeDescriptorSetLayout});

		pipeline = VKHelper::createComputePipeline(getDevice(), *layout, compShaderStageInfo);

		vkDestroyShaderModule(getDevice(), compShaderModule, nullptr);

		return pipeline;
	}

	void initDescriptor() {

		std::array<VkDescriptorPoolSize, 3> poolSize;
		poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize[0].descriptorCount = static_cast<uint32_t>(getSwapChainImageCount() * 2);
		poolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize[1].descriptorCount = static_cast<uint32_t>(getSwapChainImageCount() * 2);
		poolSize[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSize[2].descriptorCount = static_cast<uint32_t>(getSwapChainImageCount() * 2);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSize.size();
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.maxSets = static_cast<uint32_t>(getSwapChainImageCount() * 3);

		vkCreateDescriptorPool(getDevice(), &poolInfo, nullptr, &descpool);

		// TODO fix descriptor set allocation.
		std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), particleComputeDescriptorSetLayout);
		for (int i = 0; i < getSwapChainImageCount(); i++)
			layouts.push_back(particleGraphicDescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocdescInfo{};
		allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocdescInfo.descriptorPool = descpool;
		allocdescInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		allocdescInfo.pSetLayouts = layouts.data();

		std::vector<VkDescriptorSet> descSet(allocdescInfo.descriptorSetCount, VK_NULL_HANDLE);

		/*	Allocate descriptor set for both compute and graphic pipeline.	*/
		VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descSet.data()));

		particleComputeDescriptorSets.resize(getSwapChainImageCount());
		particleGraphicDescriptorSets.resize(getSwapChainImageCount());
		/*	*/
		for (int i = 0; i < getSwapChainImageCount(); i++)
			particleComputeDescriptorSets[i] = descSet[i];
		for (int i = 0; i < getSwapChainImageCount(); i++)
			particleGraphicDescriptorSets[i] = descSet[getSwapChainImageCount() + i];

		assert(particleComputeDescriptorSets.size() == getSwapChainImageCount());
		assert(particleGraphicDescriptorSets.size() == getSwapChainImageCount());

		for (size_t i = 0; i < particleComputeDescriptorSets.size(); i++) {
			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			VkDescriptorBufferInfo bufferParticleInfo{};
			bufferParticleInfo.buffer = particleBuffers[0];
			bufferParticleInfo.offset = 0;
			bufferParticleInfo.range = sizeof(Particle) * nrParticles;

			VkDescriptorBufferInfo bufferUniformInfo{};
			bufferUniformInfo.buffer = uniformBuffers[i];
			bufferUniformInfo.offset = 0;
			bufferUniformInfo.range = UniformParamMemSize;

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = particleComputeDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = nullptr;
			descriptorWrites[0].pBufferInfo = &bufferParticleInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = particleComputeDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = nullptr;
			descriptorWrites[1].pBufferInfo = &bufferUniformInfo;

			vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
								   descriptorWrites.data(), 0, nullptr);
		}

		for (size_t i = 0; i < particleGraphicDescriptorSets.size(); i++) {
			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = UniformParamMemSize;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = VK_NULL_HANDLE;
			imageInfo.sampler = sampler;

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = particleGraphicDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = nullptr;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = particleGraphicDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = nullptr;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	virtual void Initialize() {

		// TODO improve memory to align with the required by the driver
		this->UniformParamMemSize +=
			UniformParamMemSize % getVKDevice()->getPhysicalDevices()[0]->getDeviceLimits().nonCoherentAtomSize;

		const VkDeviceSize particleGraphicBufferSize = UniformParamMemSize * getSwapChainImageCount();

		const VkPhysicalDeviceMemoryProperties &memProperties =
			this->getVKDevice()->getPhysicalDevice(0)->getMemoryProperties();

		/*	Create pipelines.	*/
		this->particleSim = createComputePipeline(&particleSimLayout);
		this->particleGraphicPipeline = createGraphicPipeline(&particleGraphicLayout);

		/*	Generate uniform buffers. */
		this->uniformBuffers.resize(getSwapChainImageCount());
		this->uniformBuffersMemories.resize(getSwapChainImageCount());

		/*	Allocate memory.	*/
		for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {
			/*	Create uniform buffer.	*/
			VKHelper::createBuffer(this->getDevice(), particleGraphicBufferSize, memProperties,
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								   uniformBuffers[i], uniformBuffersMemories[i]);

			void *_data;
			VKS_VALIDATE(
				vkMapMemory(this->getDevice(), uniformBuffersMemories[i], 0, this->UniformParamMemSize, 0, &_data));
			mapMemory.push_back(_data);
		}

		/*  Generate ocean buffer.  */
		particleBuffers.resize(1);
		particleBufferMemory.resize(1);
		const VkDeviceSize particle_buffer_size = sizeof(Particle) * nrParticles;
		/*	Create particle buffer, on local device memory only.	*/
		VKHelper::createBuffer(getDevice(), particle_buffer_size, memProperties,
							   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
							   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, particleBuffers[0], particleBufferMemory[0]);
		// TODO fix.
		/*	Create init buffer to transfer.	*/
		std::vector<VkCommandBuffer> cmds =
			this->getVKDevice()->allocateCommandBuffers(getGraphicCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		VKS_VALIDATE(vkBeginCommandBuffer(cmds[0], &beginInfo));

		vkCmdFillBuffer(cmds[0], particleBuffers[0], 0, particle_buffer_size, 0);

		vkEndCommandBuffer(cmds[0]);
		this->getVKDevice()->submitCommands(getDefaultGraphicQueue(), cmds);

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
		vkFreeCommandBuffers(getDevice(), getGraphicCommandPool(), cmds.size(), cmds.data());

		initDescriptor();

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
		this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 100.0f);
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = glm::mat4(1.0f);
		/*	*/

		for (uint32_t i = 0; i < getNrCommandBuffers(); i++) {
			VkCommandBuffer cmd = getCommandBuffers(i);

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
			renderPassInfo.clearValueCount = clearValues.size();
			renderPassInfo.pClearValues = clearValues.data();

			/*	Update particles.	*/
			// TODO validate memory barrier, being not read for rendering.
			VKHelper::bufferBarrier(cmd, 0, VK_ACCESS_SHADER_WRITE_BIT, particleBuffers[0],
									sizeof(Particle) * nrParticles, 0, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
									VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, particleSim);
			// Bind descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->particleSimLayout, 0, 1,
									&this->particleComputeDescriptorSets[i], 0, nullptr);

			/*	Update particle simulation.	*/
			vkCmdDispatch(cmd, nrParticles / localInvokation, 1, 1);

			// // Barrier between compute and vertex
			// // TODO validate memory barrier. make sure that the particles has been updated before being read.
			VKHelper::bufferBarrier(cmd, 0, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, particleBuffers[0],
									sizeof(Particle) * nrParticles, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
									VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = {
				.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			/*	Setup particle rendering pipeline.	*/
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicPipeline);
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(cmd, 0, 1, &particleBuffers[0], offsets);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicLayout, 0, 1,
									&particleGraphicDescriptorSets[i], 0, nullptr);
			// Draw particles
			vkCmdDraw(cmd, nrParticles, 1, 0, 0);

			vkCmdEndRenderPass(cmd);

			// VKHelper::bufferBarrier(cmd, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, 0, particleBuffers[0],
			// 						sizeof(Particle) * nrParticles, 0, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			// 						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
		}
	}
	virtual void draw() override {

		cameraController.update(this->getTimer().deltaTime());

		/*	*/
		this->mvp.model = glm::mat4(1.0f);
		this->mvp.view = cameraController.getViewMatrix();
		this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));
		this->mvp.delta = getTimer().deltaTime();
		this->mvp.speed = 1.0f;

		/*	Copy uniform memory.	*/
		memcpy(mapMemory[getCurrentFrameIndex()], &mvp, this->UniformParamMemSize);

		/* Setup the range	*/
		VkMappedMemoryRange stagingRange{};
		stagingRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		stagingRange.memory = uniformBuffersMemories[getCurrentFrameIndex()];
		stagingRange.offset = 0;
		stagingRange.size = this->UniformParamMemSize;
		vkFlushMappedMemoryRanges(getDevice(), 1, &stagingRange);
	}
};

int main(int argc, const char **argv) {
	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, true},
																		   {"VK_KHR_xlib_surface", true}};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME, true}};

	// TODO add custom argument options for adding path of the texture and what type.
	try {
		VKSampleWindow<Ocean> particleSystem(argc, argv, required_device_extensions, {}, required_instance_extensions);
		particleSystem.run();

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}