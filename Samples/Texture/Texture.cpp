#include "Util/MeshProcedural.h"
#include "VKDataStructure.h"
#include "vulkan/vulkan_core.h"
#include <Importer/ImageImport.h>
#include <Util/CameraController.h>
#include <VKSample.h>
#include <VKWindow.h>
#include <VksCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class SingleTexture : public VKBaseSampleWindow {
	  private:
		MeshObject cubeMesh;
		Texture texture;

		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		/*	*/
		std::vector<VkDescriptorSet> descriptorSets;
		VkBuffer uniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
		std::vector<void *> mapMemory;

		CameraController camera;

		struct UniformBufferBlock {
			glm::mat4 modelViewProjection;
		} uniformStageBuffer{};

		VkDeviceSize uniformBufferSize = sizeof(UniformBufferBlock);

		const std::string vertexShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag.spv";

	  public:
		SingleTexture(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle("Texture");
			this->show();

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));

			this->camera.enableNavigation(true);
			this->camera.enableLook(true);
		}

		void release() override {

			/*	*/
			vkDestroySampler(getDevice(), sampler, nullptr);

			/*	*/
			vkDestroyBuffer(getDevice(), uniformBuffer, nullptr);
			vkUnmapMemory(getDevice(), uniformBufferMemory);
			vkFreeMemory(getDevice(), uniformBufferMemory, nullptr);

			/*	*/
			vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
			vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
		}

		VkPipeline createGraphicPipeline() {

			const auto vertShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const auto fragShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

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

			const std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo,
																			   fragShaderStageInfo};

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(fragcore::ProceduralGeometry::Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

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

			VKHelper::createDescriptorSetLayout(getDevice(), descriptorSetLayout,
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
			scissor.extent.width = static_cast<uint32_t>(this->width());
			scissor.extent.height = static_cast<uint32_t>(this->height());

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

			VKS_VALIDATE(vkCreateGraphicsPipelines(getDevice(), getPipelineCache(), 1, &pipelineInfo, nullptr,
												   &graphicsPipeline));

			vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
			vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

			return graphicsPipeline;
		}

		void Initialize() override {

			/*	*/
			const std::string texturePath = this->getResult()["texture"].as<std::string>();

			{
				ImageImporter imageImporter(this->getFileSystem(), *this);

				imageImporter.loadTexture2D(texturePath.c_str(), texture, ColorSpace::RawLinear);

				this->texture.imageView = VKHelper::createImageView(getDevice(), this->texture.image,
																	VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_B8G8R8A8_SRGB,
																	VK_IMAGE_ASPECT_COLOR_BIT, this->texture.mipLevels);

				VKHelper::createSampler(getDevice(), sampler, 0);
			}

			/*	Allocate uniform buffer.	*/
			this->uniformBufferSize = sizeof(UniformBufferBlock);
			const size_t minMapBufferSize = getPhysicalDevice()->getDeviceLimits().minUniformBufferOffsetAlignment;
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, minMapBufferSize);

			const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			const VkMemoryPropertyFlags memoryFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |	/*	*/
													 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | /*	*/
													 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	/*	*/
			this->allocateBuffer(this->uniformBufferSize, usageFlags, memoryFlag, this->uniformBuffer,
								 this->uniformBufferMemory);

			uint8_t *_data = nullptr;
			VKS_VALIDATE(vkMapMemory(getDevice(), this->uniformBufferMemory, 0,
									 this->uniformBufferSize * this->getSwapChainImageCount(), 0, (void **)&_data));

			for (size_t index = 0; index < this->getSwapChainImageCount(); index++) {
				mapMemory.push_back((void *)&_data[this->uniformBufferSize * index]);
			}

			/*	Create pipeline.	*/
			this->graphicsPipeline = createGraphicPipeline();

			/*	*/
			std::vector<VkDescriptorSetLayout> layouts(this->getSwapChainImageCount(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocdescInfo{};
			allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocdescInfo.descriptorPool = getDescriptorPool();
			allocdescInfo.descriptorSetCount = getSwapChainImageCount();
			allocdescInfo.pSetLayouts = layouts.data();

			descriptorSets.resize(getSwapChainImageCount());
			VKS_VALIDATE(vkAllocateDescriptorSets(getDevice(), &allocdescInfo, descriptorSets.data()));

			for (size_t i = 0; i < getSwapChainImageCount(); i++) {
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = uniformBuffer;
				bufferInfo.offset = uniformBufferSize * i;
				bufferInfo.range = uniformBufferSize;

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = texture.imageView;
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

				vkUpdateDescriptorSets(getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
									   descriptorWrites.data(), 0, nullptr);
			}

			{
				MeshProcedural procedural(*this);
				procedural.loadCube(cubeMesh, 1);
			}

			/*	*/
			this->uniformStageBuffer.modelViewProjection = glm::mat4(1.0f);

			this->onResize(this->width(), this->height());
		}

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

			/*	*/
			for (size_t i = 0; i < getNrCommandBuffers(); i++) {

				VkCommandBuffer cmd = getCommandBuffers(i);

				vkResetCommandBuffer(cmd, 0);

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

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				const VkViewport viewport = {
					.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
				vkCmdSetViewport(cmd, 0, 1, &viewport);
				const VkRect2D scissor = {.offset = {0, 0},
										  .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};
				vkCmdSetScissor(cmd, 0, 1, &scissor);

				/*	*/
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				/*	*/
				const VkBuffer vertexBuffers[] = {this->cubeMesh.vertexBuffer};
				const VkDeviceSize offsets[] = {this->cubeMesh.vertex_offset};
				vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(cmd, this->cubeMesh.indicesBuffer, this->cubeMesh.indices_offset,
									 VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i],
										0, nullptr);

				vkCmdDrawIndexed(cmd, this->cubeMesh.nrIndicesElements, 1, 0, 0, 0);

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
			this->uniformStageBuffer.modelViewProjection =
				(this->camera.getProjectionMatrix() * this->camera.getViewMatrix());

			memcpy(mapMemory[this->getCurrentFrameIndex()], &uniformStageBuffer,
				   (size_t)sizeof(this->uniformStageBuffer));
		}

		void update() override {}
	};

	class SingleTextureVKSample : public VKSample<SingleTexture> {
	  public:
		SingleTextureVKSample() : VKSample<SingleTexture>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/uv-texture.png"));
		}
	};

} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		vksample::SingleTextureVKSample sample;

		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}