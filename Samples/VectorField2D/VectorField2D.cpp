#include "Importer/ImageImport.h"
#include "Util/CameraController.h"
#include "VKDataStructure.h"
#include "VKSample.h"
#include "vulkan/vulkan_core.h"
#include <Util/IOUtil.h>
#include <VKWindow.h>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace vksample {

	/**
	 *
	 */
	class VectorField2D : public VKBaseSampleWindow {
	  private:
		using Motion = struct motion_t {
			glm::vec2 normalizedPos{}; /*  Position in pixel space.    */
			glm::vec2 velocity{};		 /*  direction and magnitude of mouse movement.  */
			float radius = 10.0f;	 /*  Radius of incluense, also the pressure of input.    */
			float amplitude = 1.0;
			float noise = 0;
			float pad2 = 0;
		};

		using ParticleSetting = struct particle_setting_t {
			glm::uvec4 particleBox = glm::uvec4(256, 256, 1, 0);
			glm::uvec4 vectorfieldbox = glm::uvec4(32, 32, 32, 0); // Dummy

			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
			float strength = 1.0f;

			float density = 1.0f;
			uint32_t nrparticles{};
			float spriteSize = 0.25f;
			float dragMag = 0.1f;
		};

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};
			glm::vec4 color = glm::vec4(1, 0.1, 0.1, 1);

			/*	*/
			ParticleSetting particleSetting;
			Motion motion;

			/*	*/
			float delta{};

		} uniformStageBuffer;

		using Particle = struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity, mass	*/
		};

		using VectorForce = struct vector_force_t {
			glm::vec3 position; /*	*/
			glm::vec3 force;	/*	*/
		};

		UBOObject ParticleBuffer{};

		VkBuffer particleBuffer{};
		VkDeviceMemory particleBufferMemory{};

		/*	*/
		UBOObject UniformBuffer{};
		std::vector<void *> mapMemory;
		size_t UniformParamMemSize = sizeof(uniformStageBuffer);

		/*	*/
		VkSampler sampler = VK_NULL_HANDLE;
		Texture texture{};

		/*	Compute pipeline.	*/
		VkPipeline particleSim = VK_NULL_HANDLE;
		VkPipelineLayout particleSimLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout particleComputeDescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> particleComputeDescriptorSets;

		/*	Graphic pipelines.	*/
		VkPipeline particleGraphicPipeline = VK_NULL_HANDLE;
		VkPipelineLayout particleGraphicLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout particleGraphicDescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> particleGraphicDescriptorSets;

		/*	*/
		CameraController cameraController;

		std::array<size_t, 3> localWorkGroupSize{};
		std::array<size_t, 3> localWorkGroupSize0{};

		const size_t particle_multiple_count = 8;
		/*	*/
		size_t nrParticles = 0;
		const size_t nrParticleBuffers = 2;
		size_t ParticleMemorySize = 0;

		/*	Particle.	*/
		const std::string particleVertexShaderPath = "Shaders/vectorfield/particle.vert.spv";
		const std::string particleGeometryShaderPath = "Shaders/vectorfield/particle.geom.spv";
		const std::string particleFragmentShaderPath = "Shaders/vectorfield/particle.frag.spv";

		const std::string particleInitComputeShaderPath = "Shaders/vectorfield/init_particle2D.comp.spv";
		/*	Particle Simulation in Vector Field.	*/
		const std::string particleComputeShaderPath = "Shaders/vectorfield/particle2D.comp.spv";
		/*	Particle Simulation in Vector Field.	*/
		const std::string particleMotionForceComputeShaderPath = "Shaders/vectorfield/apply_force_2D.comp.spv";

		/*	Motion vector graphic shader.	*/
		const std::string vectorFieldVertexShaderPath = "Shaders/vectorfield/vectorField.vert.spv";
		const std::string vectorFieldGeometryShaderPath = "Shaders/vectorfield/motion2D.geom.spv";
		const std::string vectorFieldFragmentPath = "Shaders/vectorfield/vectorField.frag.spv";

	  public:
		VectorField2D(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle("VectorField2D");
			this->show();
		}

		~VectorField2D() override = default;

		void release() override {

			vkDestroySampler(getDevice(), sampler, nullptr);

			/*	*/
			vkDestroyBuffer(getDevice(), particleBuffer, nullptr);
			vkFreeMemory(getDevice(), particleBufferMemory, nullptr);

			/*	*/
			vkDestroyDescriptorSetLayout(getDevice(), particleGraphicDescriptorSetLayout, nullptr);
			vkDestroyPipeline(getDevice(), particleGraphicPipeline, nullptr);
			vkDestroyPipelineLayout(getDevice(), particleGraphicLayout, nullptr);
			/*	*/
			vkDestroyDescriptorSetLayout(getDevice(), particleComputeDescriptorSetLayout, nullptr);
			vkDestroyPipeline(getDevice(), particleSim, nullptr);
			vkDestroyPipelineLayout(getDevice(), particleSimLayout, nullptr);
		}

		VkPipeline createGraphicPipeline(VkPipelineLayout *layout) {

			VkPipeline graphicsPipeline = nullptr;

			const auto vertShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->particleVertexShaderPath, this->getFileSystem());
			const auto fragShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->particleFragmentShaderPath, this->getFileSystem());

			const VkShaderModule vertShaderModule = VKHelper::createShaderModule(getDevice(), vertShaderCode);
			const VkShaderModule fragShaderModule = VKHelper::createShaderModule(getDevice(), fragShaderCode);

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

			const PipelineLayoutUtil::InputLayout input_layout = PipelineLayoutUtil::getInputLayout(vertShaderCode);

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Particle);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = 16;

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

			VKHelper::createDescriptorSetLayout(this->getDevice(), particleGraphicDescriptorSetLayout,
												{uboLayoutBinding, samplerLayoutBinding}, 0);

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
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
												  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

			VKHelper::createPipelineLayout(getDevice(), *layout, 0, {particleComputeDescriptorSetLayout});

			std::array<VkDynamicState,2> dynamicStateEnables{};
			dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamicStateEnables[1] = VK_DYNAMIC_STATE_SCISSOR;


			VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
			dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateInfo.pNext = nullptr;
			dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateInfo.dynamicStateCount = dynamicStateEnables.size();

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

			VKS_VALIDATE(vkCreateGraphicsPipelines(getDevice(), getPipelineCache(), 1, &pipelineInfo, nullptr,
												   &graphicsPipeline));

			vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
			vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

			return graphicsPipeline;
		}

		VkPipeline createComputePipeline(VkPipelineLayout *layout) {
			VkPipeline pipeline = nullptr;

			auto compShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->particleComputeShaderPath, this->getFileSystem());

			VkShaderModule compShaderModule = VKHelper::createShaderModule(getDevice(), compShaderCode);

			VkPipelineShaderStageCreateInfo compShaderStageInfo{};
			compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			compShaderStageInfo.module = compShaderModule;
			compShaderStageInfo.pName = "main";

			/*	Set descriptior layout of pipeline.	*/
			std::array<VkDescriptorSetLayoutBinding, 3> uboLayoutBindings{};
			/*	*/
			uboLayoutBindings[0].binding = 1;
			uboLayoutBindings[0].descriptorCount = 1;
			uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uboLayoutBindings[0].pImmutableSamplers = nullptr;
			uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			uboLayoutBindings[1].binding = 2;
			uboLayoutBindings[1].descriptorCount = 1;
			uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			uboLayoutBindings[1].pImmutableSamplers = nullptr;
			uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			uboLayoutBindings[2].binding = 0;
			uboLayoutBindings[2].descriptorCount = 1;
			uboLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBindings[2].pImmutableSamplers = nullptr;
			uboLayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			/*	*/
			VKHelper::createDescriptorSetLayout(this->getDevice(), particleComputeDescriptorSetLayout, 0,
												uboLayoutBindings);

			/*	*/
			VKHelper::createPipelineLayout(getDevice(), *layout, 0, {particleComputeDescriptorSetLayout});

			pipeline = VKHelper::createComputePipeline(getDevice(), *layout, compShaderStageInfo);

			vkDestroyShaderModule(getDevice(), compShaderModule, nullptr);

			return pipeline;
		}

		void initDescriptor() {

			// TODO fix descriptor set allocation.
			std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), particleComputeDescriptorSetLayout);
			for (int i = 0; i < getSwapChainImageCount(); i++) {
				layouts.push_back(particleGraphicDescriptorSetLayout);
			}

			VkDescriptorSetAllocateInfo allocdescInfo{};
			allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocdescInfo.descriptorPool = getDescriptorPool();
			allocdescInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
			allocdescInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> descSet(allocdescInfo.descriptorSetCount, VK_NULL_HANDLE);

			/*	Allocate descriptor set for both compute and graphic pipeline.	*/
			VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descSet.data()));

			particleComputeDescriptorSets.resize(getSwapChainImageCount());
			particleGraphicDescriptorSets.resize(getSwapChainImageCount());
			/*	*/
			for (int i = 0; i < getSwapChainImageCount(); i++) {
				particleComputeDescriptorSets[i] = descSet[i];
			}
			for (int i = 0; i < getSwapChainImageCount(); i++) {
				particleGraphicDescriptorSets[i] = descSet[getSwapChainImageCount() + i];
			}

			assert(particleComputeDescriptorSets.size() == getSwapChainImageCount());
			assert(particleGraphicDescriptorSets.size() == getSwapChainImageCount());

			for (size_t i = 0; i < particleComputeDescriptorSets.size(); i++) {
				std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

				VkDescriptorBufferInfo bufferReadParticleInfo{};
				bufferReadParticleInfo.buffer = particleBuffer;
				bufferReadParticleInfo.offset = (i % nrParticleBuffers) * ParticleMemorySize;
				bufferReadParticleInfo.range = ParticleMemorySize;

				VkDescriptorBufferInfo bufferWriteParticleInfo{};
				bufferWriteParticleInfo.buffer = particleBuffer;
				bufferWriteParticleInfo.offset = ((i + 1) % nrParticleBuffers) * ParticleMemorySize;
				bufferWriteParticleInfo.range = ParticleMemorySize;

				VkDescriptorBufferInfo bufferUniformInfo{};
				bufferUniformInfo.buffer = UniformBuffer.buffer;
				// bufferUniformInfo.offset = (i % getSwapChainImageCount()) * UniformParamMemSize;
				// bufferUniformInfo.range = UniformParamMemSize;

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = particleComputeDescriptorSets[i];
				descriptorWrites[0].dstBinding = 1;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = nullptr;
				descriptorWrites[0].pBufferInfo = &bufferReadParticleInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = particleComputeDescriptorSets[i];
				descriptorWrites[1].dstBinding = 2;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = nullptr;
				descriptorWrites[1].pBufferInfo = &bufferWriteParticleInfo;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = particleComputeDescriptorSets[i];
				descriptorWrites[2].dstBinding = 0;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = nullptr;
				descriptorWrites[2].pBufferInfo = &bufferUniformInfo;

				vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
									   descriptorWrites.data(), 0, nullptr);
			}

			for (size_t i = 0; i < particleGraphicDescriptorSets.size(); i++) {
				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				VkDescriptorBufferInfo bufferInfo{};
				// bufferInfo.buffer = uniformBuffer;
				// bufferInfo.offset = i * UniformParamMemSize;
				// bufferInfo.range = UniformParamMemSize;

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

		void Initialize() override {

			/*	*/
			const std::string particleTexturePath = this->getResult()["texture"].as<std::string>();

			{
				ImageImporter imageImporter(this->getFileSystem(), *this);

				imageImporter.loadTexture2D(particleTexturePath.c_str(), texture, ColorSpace::RawLinear);

				this->texture.imageView =
					VKHelper::createImageView(getDevice(), this->texture.image, VK_IMAGE_VIEW_TYPE_2D,
											  VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

				VKHelper::createSampler(getDevice(), sampler, 0);
			}

			this->UniformParamMemSize =
				Math::align(sizeof(uniformStageBuffer),
							this->getPhysicalDevice()->getDeviceLimits().minUniformBufferOffsetAlignment);

			const VkDeviceSize particleUniformBufferSize = UniformParamMemSize * getSwapChainImageCount();

			/*	Create pipelines.	*/
			this->particleSim = createComputePipeline(&particleSimLayout);
			this->particleGraphicPipeline = createGraphicPipeline(&particleGraphicLayout);

			VkBufferUsageFlags usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
											 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			this->allocateBuffer(particleUniformBufferSize, usageFlag, memFlags, ParticleBuffer.buffer,
								 ParticleBuffer.memory);

			uint8_t *_data = nullptr;
			VKS_VALIDATE(vkMapMemory(this->getDevice(), UniformBuffer.memory, this->UniformParamMemSize,
									 this->UniformParamMemSize, 0, (void**)&_data));
			/*	Allocate memory.	*/
			for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {

				mapMemory.push_back(_data);
			}

			/*	Create particle buffer, on local device memory only.	*/
			// VKHelper::createBuffer(getDevice(), ParticleMemorySize * nrParticleBuffers, memProperties,
			// 					   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			// 					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, particleBuffer, particleBufferMemory);

			initDescriptor();

			this->onResize(width(), height());
		}

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultTransferQueue()));
			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

			/*	*/

			for (uint32_t i = 0; i < this->getNrCommandBuffers(); i++) {
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
				VKHelper::bufferBarrier(cmd, 0, VK_ACCESS_SHADER_WRITE_BIT, particleBuffer, ParticleMemorySize,
										(i % nrParticleBuffers) * ParticleMemorySize,
										VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, particleSim);
				// Bind descriptor
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->particleSimLayout, 0, 1,
										&this->particleComputeDescriptorSets[i], 0, nullptr);

				/*	Update particle simulation.	*/
				//vkCmdDispatch(cmd, nrParticles / localInvokation, 1, 1);

				// // Barrier between compute and vertex
				// // TODO validate memory barrier. make sure that the particles has been updated before being read.
				VKHelper::bufferBarrier(cmd, 0, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, particleBuffer, ParticleMemorySize,
										(i % nrParticleBuffers) * ParticleMemorySize,
										VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = {
					.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
				vkCmdSetViewport(cmd, 0, 1, &viewport);

				/*	Setup particle rendering pipeline.	*/
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicPipeline);
				VkDeviceSize offsets[] = {(i % nrParticleBuffers) * ParticleMemorySize};
				vkCmdBindVertexBuffers(cmd, 0, 1, &particleBuffer, offsets);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicLayout, 0, 1,
										&particleGraphicDescriptorSets[i], 0, nullptr);
				// Draw particles
				vkCmdDraw(cmd, nrParticles, 1, 0, 0);

				vkCmdEndRenderPass(cmd);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}

			this->cameraController.setAspect((float)width / (float)height);
		}

		void draw() override {}

		void update() override {
			this->cameraController.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				const float xHalf = this->uniformStageBuffer.particleSetting.particleBox.x / 2.f;
				const float yHalf = this->uniformStageBuffer.particleSetting.particleBox.y / 2.f;
				glm::mat4 proj = glm::ortho(-xHalf, xHalf, -yHalf, yHalf, -10.0f, 10.0f);

				glm::mat4 viewMatrix = glm::translate(glm::vec3(-xHalf, -yHalf, 0));
				/*	*/
				this->uniformStageBuffer.proj = proj;

				this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = viewMatrix;
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

				if (this->getInput().getMousePressed(Input::MouseButton::LEFT_BUTTON)) {
					int x = 0, y = 0;
					this->getInput().getMousePosition(&x, &y);
					this->uniformStageBuffer.motion.normalizedPos =
						glm::vec2(1, 1) - (glm::vec2(x, y) / glm::vec2(this->width(), this->height()));
					this->uniformStageBuffer.motion.normalizedPos.x =
						1.0f - this->uniformStageBuffer.motion.normalizedPos.x;
				}
			}

			/*	Update Memory.	*/
			/*	Copy uniform memory.	*/
			memcpy(this->mapMemory[getCurrentFrameIndex()], &uniformStageBuffer, sizeof(uniformStageBuffer));
		}
	};
} // namespace vksample

// options("T,texture", "Particle Texture Path",
// 		cxxopts::value<std::string>()->default_value("asset/particle-cell.png"));

int main(int argc, const char **argv) {
	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	// TODO add custom argument options for adding path of the texture and what type.

	try {
		VKSample<vksample::VectorField2D> VectorField2D;
		VectorField2D.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}