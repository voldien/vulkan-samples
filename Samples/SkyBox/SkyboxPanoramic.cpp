#include "Scene/CameraController.h"
#include "Util/MeshProcedural.h"
#include "VKSample.h"
#include "vulkan/vulkan_core.h"
#include <Importer/ImageImport.h>
#include <VKWindow.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace vksample {

	class SkyboxPanoramic : public VKBaseSampleWindow {
	  private:
		MeshObject cubeMesh;
		Texture PanoramicTexture;

		/*	*/
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

		/*	*/
		std::vector<VkDescriptorSet> descriptorSets;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

		VkSampler sampler = VK_NULL_HANDLE;

		/*	*/
		VkBuffer uniformBuffer{};
		VkDeviceMemory uniformBufferMemory{};
		std::vector<void *> mapMemory;
		VkDeviceSize uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController cameraController;

		const std::string vertexShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentShaderPath = "Shaders/skybox/panoramic.frag.spv";

		struct UniformBufferBlock {
			glm::mat4 proj{};
			glm::mat4 modelViewProjection{};
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			float exposure = 1.0f;
			float gamma = 2.2f;
		} uniform_stage_buffer;

	  public:
		SkyboxPanoramic(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {

			this->cameraController.enableNavigation(false);

			this->cameraController.setPosition(glm::vec3(0.0f));
			this->cameraController.lookAt(glm::vec3(1.f));

			this->setTitle("Skybox Panoramic");
			this->show();
		}
		~SkyboxPanoramic() override = default;

		void release() override {

			vkDestroySampler(getDevice(), sampler, nullptr);

			// vkDestroyImageView(getDevice(), skyboxTextureView, nullptr);
			// vkDestroyImage(getDevice(), texture, nullptr);
			// vkFreeMemory(getDevice(), textureMemory, nullptr);

			vkDestroyBuffer(getDevice(), uniformBuffer, nullptr);
			vkUnmapMemory(getDevice(), uniformBufferMemory);
			vkFreeMemory(getDevice(), uniformBufferMemory, nullptr);

			vkDestroyDescriptorSetLayout(getDevice(), descriptorSetLayout, nullptr);
			vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);
		}

		VkPipeline createGraphicPipeline() {

			const auto vertShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const auto fragShaderCode =
				fragcore::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

			const VkShaderModule vertShaderModule = VKHelper::createShaderModule(this->getDevice(), vertShaderCode);
			const VkShaderModule fragShaderModule = VKHelper::createShaderModule(this->getDevice(), fragShaderCode);

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

			const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo,
																				 fragShaderStageInfo};

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(fragcore::ProceduralGeometry::Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

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
			uboLayoutBinding.binding = 1;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.pImmutableSamplers = nullptr;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = 0;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VKHelper::createDescriptorSetLayout(this->getDevice(), descriptorSetLayout,
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
			depthStencil.depthTestEnable = VK_FALSE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VKHelper::createPipelineLayout(this->getDevice(), pipelineLayout, 0, {descriptorSetLayout});

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

			VKS_VALIDATE(vkCreateGraphicsPipelines(getDevice(), this->getPipelineCache(), 1, &pipelineInfo,
												   getAllocatorCallback(), &graphicsPipeline));

			vkDestroyShaderModule(getDevice(), fragShaderModule, nullptr);
			vkDestroyShaderModule(getDevice(), vertShaderModule, nullptr);

			return graphicsPipeline;
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			/*	Load and Create Texture.	*/
			ImageImporter imageImporter(this->getFileSystem(), *this);

			imageImporter.loadTexture2D(panoramicPath.c_str(), this->PanoramicTexture, ColorSpace::RawLinear);

			this->PanoramicTexture.imageView =
				VKHelper::createImageView(getDevice(), this->PanoramicTexture.image, VK_IMAGE_VIEW_TYPE_2D,
										  VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			VKHelper::createSampler(getDevice(), sampler, 0);

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
			graphicsPipeline = createGraphicPipeline();

			/*	*/
			std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocdescInfo{};
			allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocdescInfo.descriptorPool = getDescriptorPool();
			allocdescInfo.descriptorSetCount = getSwapChainImageCount();
			allocdescInfo.pSetLayouts = layouts.data();

			descriptorSets.resize(this->getSwapChainImageCount());
			VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descriptorSets.data()));

			for (size_t i = 0; i < getSwapChainImageCount(); i++) {
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = uniformBuffer;
				bufferInfo.offset = uniformBufferSize * i;
				bufferInfo.range = uniformBufferSize;

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = this->PanoramicTexture.imageView;
				imageInfo.sampler = sampler;

				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = descriptorSets[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = &imageInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = descriptorSets[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
									   descriptorWrites.data(), 0, nullptr);
			}

			/*	*/
			{
				MeshProcedural procedural(*this);
				procedural.loadCube(cubeMesh, 1);
			}

			this->onResize(this->width(), this->height());
		}

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(this->getDefaultTransferQueue()));
			VKS_VALIDATE(vkQueueWaitIdle(this->getDefaultGraphicQueue()));

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
				clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
				clearValues[1].depthStencil = {1.0f, 0};

				renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
				renderPassInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				const VkViewport viewport = {.x = 0,
											 .y = static_cast<float>(height),
											 .width = (float)width,
											 .height = (float)-height,
											 .minDepth = 0,
											 .maxDepth = 1.0f};
				vkCmdSetViewport(cmd, 0, 1, &viewport);
				const VkRect2D scissor = {.offset = {0, 0},
										  .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};
				vkCmdSetScissor(cmd, 0, 1, &scissor);

				/*	*/
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				/*	*/
				const VkBuffer vertexBuffers[] = {this->cubeMesh.vertexBuffer};
				const VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(cmd, this->cubeMesh.indicesBuffer, this->cubeMesh.indices_offset,
									 VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i],
										0, nullptr);

				vkCmdDrawIndexed(cmd, this->cubeMesh.nrIndicesElements, 1, 0, 0, 0);

				vkCmdEndRenderPass(cmd);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}

			this->cameraController.setAspect((float)width / (float)height);
		}

		void draw() override {

			this->cameraController.update(this->getTimer().deltaTime<float>());
			// TODO add character controller.

			this->uniform_stage_buffer.proj = this->cameraController.getProjectionMatrix();
			this->uniform_stage_buffer.modelViewProjection =
				(cameraController.getProjectionMatrix() * glm::inverse(cameraController.getRotationMatrix()));

			// Setup the range
			memcpy(mapMemory[this->getCurrentFrameIndex()], &uniform_stage_buffer,
				   (size_t)sizeof(this->uniform_stage_buffer));
		}

		void update() override {}
	};

	class SkyBoxPanoramicVKSample : public VKSample<SkyboxPanoramic> {
	  public:
		SkyBoxPanoramicVKSample() : VKSample<SkyboxPanoramic>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,skybox-texture", "Texture Path",
					cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};
} // namespace vksample

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		vksample::SkyBoxPanoramicVKSample skybox;
		skybox.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}