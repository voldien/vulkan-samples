#include "Util/Time.hpp"

#include "VksCommon.h"
#include <Importer/ImageImport.h>
#include <SDL2/SDL.h>
#include <Util/CameraController.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace vksample {

	class NormalMap : public VKWindow {
	  private:
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
		VkDescriptorPool descpool = VK_NULL_HANDLE;

		VkSampler sampler = VK_NULL_HANDLE;
		/*	*/
		VkImage diffuse_texture = VK_NULL_HANDLE;
		VkImageView diffuse_TextureView = VK_NULL_HANDLE;
		VkDeviceMemory diffuse_textureMemory = VK_NULL_HANDLE;

		/*	*/
		VkImage normal_texture = VK_NULL_HANDLE;
		VkImageView normal_texture_view = VK_NULL_HANDLE;
		VkDeviceMemory normal_textureMemory = VK_NULL_HANDLE;

		std::vector<VkDescriptorSet> descriptorSets;
		VkBuffer uniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
		std::vector<void *> mapMemory;
		VkDeviceSize uniformBufferSize;

		CameraController cameraController;

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			glm::vec4 tintColor = glm::vec4(1);
			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			float normalStrength = 1.0f;
		} mvp;

		typedef struct _vertex_t {
			float pos[3];
			float uv[2];
		} Vertex;

		const std::string diffuseTexturePath = "asset/diffuse.png";
		const std::string normalTexturePath = "asset/normalmap.png";

		const std::string vertexShaderPath = "Shaders/normalmap/normalmap.vert.spv";
		const std::string fragmentShaderPath = "Shaders/normalmap/normalmap.frag.spv";

	  public:
		NormalMap(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKWindow(core, device, -1, -1, -1, -1) {
			this->setTitle("NormalMap");
			this->show();
		}
		virtual ~NormalMap() {}

		virtual void release() override {

			vkDestroySampler(getDevice(), sampler, nullptr);

			vkDestroyImageView(getDevice(), diffuse_TextureView, nullptr);
			vkDestroyImage(getDevice(), diffuse_texture, nullptr);
			vkFreeMemory(getDevice(), diffuse_textureMemory, nullptr);

			vkDestroyImageView(getDevice(), normal_texture_view, nullptr);
			vkDestroyImage(getDevice(), normal_texture, nullptr);
			vkFreeMemory(getDevice(), normal_textureMemory, nullptr);

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

		VkPipeline createGraphicPipeline() {

			auto vertShaderCode =
				vksample::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, fragcore::FileSystem::getFileSystem());
			auto fragShaderCode = vksample::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath,
																		   fragcore::FileSystem::getFileSystem());

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

			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions;

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = 12;

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = 20;

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[3].offset = 32;

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

			VkDescriptorSetLayoutBinding samplerDiffuseLayoutBinding{};
			samplerDiffuseLayoutBinding.binding = 1;
			samplerDiffuseLayoutBinding.descriptorCount = 1;
			samplerDiffuseLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerDiffuseLayoutBinding.pImmutableSamplers = nullptr;
			samplerDiffuseLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding samplerNormalLayoutBinding{};
			samplerNormalLayoutBinding.binding = 2;
			samplerNormalLayoutBinding.descriptorCount = 1;
			samplerNormalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerNormalLayoutBinding.pImmutableSamplers = nullptr;
			samplerNormalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VKHelper::createDescriptorSetLayout(
				this->getDevice(), descriptorSetLayout,
				{uboLayoutBinding, samplerDiffuseLayoutBinding, samplerNormalLayoutBinding});

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

			/*	Load and Create Texture.	*/
			VkCommandBuffer cmd;
			std::vector<VkCommandBuffer> cmds = this->getVKDevice()->allocateCommandBuffers(
				getGraphicCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			VKS_VALIDATE(vkBeginCommandBuffer(cmds[0], &beginInfo));

			ImageImporter imageImporter(fragcore::FileSystem::getFileSystem(), *this->getVKDevice());

			/*	Diffuse Texture.	*/
			imageImporter.createImage2D(this->diffuseTexturePath.c_str(), getDevice(), getGraphicCommandPool(),
										getDefaultGraphicQueue(), physicalDevice(), this->diffuse_texture,
										this->diffuse_textureMemory);

			/*	Normal Texture.	*/
			imageImporter.createImage2D(this->normalTexturePath.c_str(), getDevice(), getGraphicCommandPool(),
										getDefaultGraphicQueue(), physicalDevice(), this->normal_texture,
										this->normal_textureMemory);

			vkEndCommandBuffer(cmds[0]);
			this->getVKDevice()->submitCommands(getDefaultGraphicQueue(), cmds);

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));
			vkFreeCommandBuffers(this->getDevice(), getGraphicCommandPool(), cmds.size(), cmds.data());

			this->diffuse_TextureView =
				VKHelper::createImageView(this->getDevice(), this->diffuse_texture, VK_IMAGE_VIEW_TYPE_2D,
										  VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			this->normal_texture_view =
				VKHelper::createImageView(this->getDevice(), this->normal_texture, VK_IMAGE_VIEW_TYPE_2D,
										  VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			VKHelper::createSampler(this->getDevice(), sampler);

			/*	Compute uniform buffer size, in respect to the alignment requirement.	*/
			this->uniformBufferSize = sizeof(UniformBufferBlock);
			const size_t minMapBufferSize =
				this->getVKDevice()->getPhysicalDevices()[0]->getDeviceLimits().minUniformBufferOffsetAlignment;
			this->uniformBufferSize = fragcore::Math::align(uniformBufferSize, minMapBufferSize);

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(this->physicalDevice(), &memProperties);

			VKHelper::createBuffer(getDevice(), uniformBufferSize * this->getSwapChainImageCount(), memProperties,
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								   uniformBuffer, uniformBufferMemory);

			for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {
				void *_data;
				VKS_VALIDATE(vkMapMemory(getDevice(), this->uniformBufferMemory, this->uniformBufferSize * i,
										 this->uniformBufferSize, 0, &_data));
				mapMemory.push_back(_data);
			}

			/*	Create pipeline.	*/
			graphicsPipeline = createGraphicPipeline();

			/*	Allocate descriptor set.	*/
			const std::vector<VkDescriptorPoolSize> poolSize = {
				{
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					static_cast<uint32_t>(this->getSwapChainImageCount()),
				},
				{
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					static_cast<uint32_t>(this->getSwapChainImageCount() * 2),
				}};
			descpool = VKHelper::createDescPool(this->getDevice(), poolSize, this->getSwapChainImageCount() * 3);

			/*	*/
			std::vector<VkDescriptorSetLayout> layouts(this->getSwapChainImageCount(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocdescInfo{};
			allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocdescInfo.descriptorPool = descpool;
			allocdescInfo.descriptorSetCount = static_cast<uint32_t>(getSwapChainImageCount());
			allocdescInfo.pSetLayouts = layouts.data();

			descriptorSets.resize(this->getSwapChainImageCount());
			VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descriptorSets.data()));

			for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = this->uniformBuffer;
				bufferInfo.offset = this->uniformBufferSize * i;
				bufferInfo.range = this->uniformBufferSize;

				VkDescriptorImageInfo imageDiffuseInfo{};
				imageDiffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageDiffuseInfo.imageView = diffuse_TextureView;
				imageDiffuseInfo.sampler = sampler;

				VkDescriptorImageInfo imageNormalInfo{};
				imageNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageNormalInfo.imageView = normal_texture_view;
				imageNormalInfo.sampler = sampler;

				std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

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
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = &imageNormalInfo;

				vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
									   descriptorWrites.data(), 0, nullptr);
			}

			/*	*/
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
			this->mvp.proj = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.15f, 100.0f);

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

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i],
										0, nullptr);

				vkCmdDraw(cmd, vertices.size(), 1, 0, 0);

				vkCmdEndRenderPass(cmd);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}
		}

		virtual void draw() override {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->cameraController.update(getTimer().deltaTime());

			/*	*/
			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model =
				glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(10.95f));
			this->mvp.view = this->cameraController.getViewMatrix();
			this->mvp.modelViewProjection = this->mvp.proj * this->mvp.view * this->mvp.model;
			this->mvp.ViewProj = this->mvp.proj * this->mvp.view;

			// Setup the range
			memcpy(mapMemory[this->getCurrentFrameIndex()], &mvp, (size_t)sizeof(this->mvp));
		}

		virtual void update() {}
	};
} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};
	// TODO add custom argument options for adding path of the texture and what type.
	try {
		VKSample<vksample::NormalMap> sample;
		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}