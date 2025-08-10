#include "VKSample.h"
#include "vulkan/vulkan_core.h"
#include <Importer/ImageImport.h>
#include <SDL2/SDL.h>
#include <Util/CameraController.h>
#include <VKWindow.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class NormalMap : public VKBaseSampleWindow {
	  private:
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexIndicesMemory = VK_NULL_HANDLE;
		VkDeviceSize indices_offset = 0;
		size_t nrIndices = 1;
		VkDeviceMemory vertexMemory = VK_NULL_HANDLE;

		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

		VkSampler sampler = VK_NULL_HANDLE;
		Texture DiffuseTexture;
		Texture NormalTexture;

		std::vector<VkDescriptorSet> descriptorSets;
		VkBuffer uniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
		std::vector<void *> mapMemory;
		VkDeviceSize uniformBufferSize{};

		CameraController camera;

		struct UniformBufferBlock {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 ViewProj{};
			glm::mat4 modelViewProjection{};

			glm::vec4 tintColor = glm::vec4(1);
			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			float normalStrength = 1.0f;
		} mvp;

		using Vertex = struct _vertex_t {
			float pos[3];
			float uv[2];
		};

		const std::string diffuseTexturePath = "asset/diffuse.png";
		const std::string normalTexturePath = "asset/normalmap.png";

		const std::string vertexShaderPath = "Shaders/normalmap/normalmap.vert.spv";
		const std::string fragmentShaderPath = "Shaders/normalmap/normalmap.frag.spv";

	  public:
		NormalMap(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle("NormalMap");
			this->show();
		}
		~NormalMap() override = default;

		void release() override {

			vkDestroySampler(getDevice(), sampler, nullptr);

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
				fragcore::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			auto fragShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

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

			const std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo,
																			   fragShaderStageInfo};

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

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
				{uboLayoutBinding, samplerDiffuseLayoutBinding, samplerNormalLayoutBinding}, 0);

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

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;

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

			VKHelper::createPipelineLayout(getDevice(), pipelineLayout, 0, {descriptorSetLayout});

			std::array<VkDynamicState, 2> dynamicStateEnables{};
			dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamicStateEnables[1] = VK_DYNAMIC_STATE_SCISSOR;
			VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
			dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateInfo.pNext = nullptr;
			dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateInfo.dynamicStateCount = dynamicStateEnables.size();

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

			VKS_VALIDATE(vkCreateGraphicsPipelines(getDevice(), this->getPipelineCache(), 1, &pipelineInfo, nullptr,
												   &graphicsPipeline));

			vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
			vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

			return graphicsPipeline;
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string normalTexturePath = this->getResult()["normal-texture"].as<std::string>();

			ImageImporter imageImporter(this->getFileSystem(), *this);

			/*	Diffuse Texture.	*/
			imageImporter.loadTexture2D(this->diffuseTexturePath.c_str(), DiffuseTexture, ColorSpace::RawLinear);

			/*	Normal Texture.	*/
			imageImporter.loadTexture2D(this->diffuseTexturePath.c_str(), NormalTexture, ColorSpace::RawLinear);

			this->DiffuseTexture.imageView =
				VKHelper::createImageView(this->getDevice(), this->DiffuseTexture.image, VK_IMAGE_VIEW_TYPE_2D,
										  VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			this->NormalTexture.imageView =
				VKHelper::createImageView(this->getDevice(), this->NormalTexture.image, VK_IMAGE_VIEW_TYPE_2D,
										  VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			VKHelper::createSampler(this->getDevice(), sampler, 0);

			/*	Compute uniform buffer size, in respect to the alignment requirement.	*/
			this->uniformBufferSize = sizeof(UniformBufferBlock);
			const size_t minMapBufferSize =
				this->getPhysicalDevice()->getDeviceLimits().minUniformBufferOffsetAlignment;
			this->uniformBufferSize = fragcore::Math::align(uniformBufferSize, minMapBufferSize);

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(this->physicalDevice(), &memProperties);

			VKHelper::createBuffer(getDevice(), uniformBufferSize * this->getSwapChainImageCount(), memProperties,
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								   uniformBuffer, uniformBufferMemory);

			this->mapMemory.resize(this->getSwapChainImageCount());
			uint8_t *_data = nullptr;
			VKS_VALIDATE(
				vkMapMemory(this->getDevice(), uniformBufferMemory, 0, this->uniformBufferSize, 0, (void **)&_data));
			for (size_t i = 0; i < this->getSwapChainImageCount(); i++) {
				this->mapMemory[i] = &_data[this->uniformBufferSize * i];
			}

			/*	Create pipeline.	*/
			graphicsPipeline = createGraphicPipeline();

			/*	*/
			std::vector<VkDescriptorSetLayout> layouts(this->getSwapChainImageCount(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocdescInfo{};
			allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocdescInfo.descriptorPool = getDescriptorPool();
			allocdescInfo.descriptorSetCount = getSwapChainImageCount();
			allocdescInfo.pSetLayouts = layouts.data();

			descriptorSets.resize(this->getSwapChainImageCount());
			VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descriptorSets.data()));

			for (size_t desc_set_index = 0; desc_set_index < this->getSwapChainImageCount(); desc_set_index++) {
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = this->uniformBuffer;
				bufferInfo.offset = this->uniformBufferSize * desc_set_index;
				bufferInfo.range = this->uniformBufferSize;

				VkDescriptorImageInfo imageDiffuseInfo{};
				imageDiffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageDiffuseInfo.imageView = DiffuseTexture.imageView;
				imageDiffuseInfo.sampler = sampler;

				VkDescriptorImageInfo imageNormalInfo{};
				imageNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageNormalInfo.imageView = NormalTexture.imageView;
				imageNormalInfo.sampler = sampler;

				std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = descriptorSets[desc_set_index];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = descriptorSets[desc_set_index];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &imageDiffuseInfo;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = descriptorSets[desc_set_index];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = &imageNormalInfo;

				vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
									   descriptorWrites.data(), 0, nullptr);
			}

			{
				/*	Load geometry.	*/
				std::vector<fragcore::ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				fragcore::ProceduralGeometry::generateCube(1.0f, vertices, indices);

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = sizeof(vertices[0]) * vertices.size() + sizeof(indices[0]) * indices.size();
				bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VKS_VALIDATE(vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &vertexBuffer));
				this->indices_offset = sizeof(vertices[0]) * vertices.size();
				this->nrIndices = indices.size();

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(getDevice(), vertexBuffer, &memRequirements);

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex =
					VKHelper::findMemoryType(physicalDevice(), memRequirements.memoryTypeBits,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
						.value();

				VKS_VALIDATE(vkAllocateMemory(getDevice(), &allocInfo, nullptr, &vertexIndicesMemory));

				VKS_VALIDATE(vkBindBufferMemory(getDevice(), vertexBuffer, vertexIndicesMemory, 0));

				/*	Upload vertex data.	*/
				uint8_t *data = nullptr;
				VKS_VALIDATE(vkMapMemory(getDevice(), vertexIndicesMemory, 0, bufferInfo.size, 0, (void **)&data));
				memcpy(data, vertices.data(), (size_t)vertices.size() * sizeof(vertices[0]));
				memcpy(data + indices_offset, indices.data(), (size_t)indices.size() * sizeof(indices[0]));
				vkUnmapMemory(getDevice(), vertexIndicesMemory);
			}

			this->onResize(this->width(), this->height());
		}

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

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
				VkRect2D scissor = {.offset = {0, 0},
									.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};
				vkCmdSetScissor(cmd, 0, 1, &scissor);

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				/*	*/
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				/*	*/
				const VkBuffer vertexBuffers[] = {vertexBuffer};
				const VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(cmd, vertexBuffer, indices_offset, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i],
										0, nullptr);

				vkCmdDrawIndexed(cmd, nrIndices, 1, 0, 0, 0);

				vkCmdEndRenderPass(cmd);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}

			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(getTimer().deltaTime<float>());

			/*	*/
			this->mvp.proj = this->camera.getProjectionMatrix();
			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model =
				glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(10.95f));
			this->mvp.view = this->camera.getViewMatrix();
			this->mvp.modelViewProjection = this->mvp.proj * this->mvp.view * this->mvp.model;
			this->mvp.ViewProj = this->mvp.proj * this->mvp.view;

			// Setup the range
			memcpy(mapMemory[this->getCurrentFrameIndex()], &mvp, (size_t)sizeof(this->mvp));
		}

		void update() override {}
	};

	class NormalMapVKSample : public VKSample<NormalMap> {
	  public:
		NormalMapVKSample() : VKSample<NormalMap>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"N,normal-texture", "NormalMap Path",
				cxxopts::value<std::string>()->default_value("asset/normalmap.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"));
		}
	};

} // namespace vksample

int main(int argc, const char **argv) {

	const std::unordered_map<const char *, bool> required_instance_extensions = {};
	const std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		vksample::NormalMapVKSample sample;
		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}