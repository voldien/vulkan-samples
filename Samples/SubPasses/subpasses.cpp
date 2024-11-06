#include "Importer/IOUtil.h"
#include <SDL2/SDL.h>
#include <VKSample.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <iostream>

namespace vksample {
	// TOOD rename
	class SubPasses : public VKWindow {
	  private:
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;

		std::vector<VkImage> multiPassImages;
		std::vector<VkImageView> multiPassImageViews;
		std::vector<VkFramebuffer> multiPassFramebuffers;

	  public:
		SubPasses(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKWindow(core, device, -1, -1, -1, -1) {
			this->show();
			this->setTitle("SubPasses");
		}

		virtual ~SubPasses() {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		const std::string vertexShaderPath = "shaders/multipass/multipass.vert.spv";
		const std::string fragmentShaderPath = "shaders/multipass/multipass.frag.spv";

		virtual void release() override {

			/*	*/
			vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
			vkFreeMemory(getDevice(), vertexMemory, nullptr);

			/*	*/
			vkDestroyPipeline(getDevice(), graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(getDevice(), pipelineLayout, nullptr);

			vkDestroyRenderPass(getDevice(), renderPass, nullptr);
		}

		/*	{vertex(3)|uv(2)}*/
		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		VkPipeline createGraphicPipeline() {

			auto vertShaderCode =
				vksample::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			auto fragShaderCode = vksample::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath,
																		   this->getFileSystem());

			VkShaderModule vertShaderModule = VKHelper::createShaderModule(this->getDevice(), vertShaderCode);
			VkShaderModule fragShaderModule = VKHelper::createShaderModule(this->getDevice(), fragShaderCode);

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
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = 8;

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
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
			depthStencil.depthWriteEnable = VK_FALSE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 0;
			pipelineLayoutInfo.pushConstantRangeCount = 0;

			VKS_VALIDATE(vkCreatePipelineLayout(getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDepthStencilState = &depthStencil;
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
			/*	Create pipeline.	*/
			graphicsPipeline = createGraphicPipeline();

			/*	Setup multipass (subpasses).	*/
			{
				VkAttachmentDescription colorAttachment{};
				colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM; // swapChainImageFormat;
				colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

				VkSubpassDescription subpass{};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

				VkRenderPassCreateInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.pAttachments = &colorAttachment;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;

				VKS_VALIDATE(vkCreateRenderPass(getDevice(), &renderPassInfo, nullptr, &renderPass));
			}

			/*	Allocate buffer for the triangle.	*/
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = sizeof(vertices[0]) * vertices.size();
			bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VKS_VALIDATE(vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &vertexBuffer));

			/*	*/
			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(getDevice(), vertexBuffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex =
				getVKDevice()
					->findMemoryType(memRequirements.memoryTypeBits,
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
					.value();

			/*	Allocate memory that will be used for the buffer.	*/
			VKS_VALIDATE(vkAllocateMemory(getDevice(), &allocInfo, nullptr, &vertexMemory));

			/*	Bind the vertex buffer with the memory that contains the triangle vertices data.	*/
			VKS_VALIDATE(vkBindBufferMemory(getDevice(), vertexBuffer, vertexMemory, 0));

			void *data;
			VKS_VALIDATE(vkMapMemory(getDevice(), vertexMemory, 0, bufferInfo.size, 0, &data));
			memcpy(data, vertices.data(), (size_t)bufferInfo.size);
			vkUnmapMemory(getDevice(), vertexMemory);

			onResize(width(), height());
		}

		virtual void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(this->getDefaultGraphicQueue()));

			for (uint32_t i = 0; i < this->getNrCommandBuffers(); i++) {
				VkCommandBuffer cmd = this->getCommandBuffers(i);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				/*	Multipass render pass.	*/
				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = renderPass;
				renderPassInfo.framebuffer = multiPassFramebuffers[i];
				renderPassInfo.renderArea.offset = {0, 0};
				renderPassInfo.renderArea.extent.width = width;
				renderPassInfo.renderArea.extent.height = height;

				/*	*/
				std::array<VkClearValue, 2> clearValues{};
				clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
				clearValues[1].depthStencil = {1.0f, 0};
				renderPassInfo.clearValueCount = clearValues.size();
				renderPassInfo.pClearValues = clearValues.data();

				VkViewport viewport = {
					.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
				vkCmdSetViewport(cmd, 0, 1, &viewport);

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				/*	*/
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				// Render Scene.
				{
					VkBuffer vertexBuffers[] = {vertexBuffer};
					VkDeviceSize offsets[] = {0};
					vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

					vkCmdDraw(cmd, 3, 1, 0, 0);
				}

				vkCmdEndRenderPass(cmd);

				/*	Transfer each target to default framebuffer.	*/
				const float halfW = (width / 2.0f);
				const float halfH = (height / 2.0f);
				for (size_t i = 0; i < this->multiPassImages.size(); i++) {
					/*	*/
					VkImageBlit blitRegion{};
					blitRegion.srcOffsets[1].x = width;
					blitRegion.srcOffsets[1].y = height;
					blitRegion.srcOffsets[1].z = 1;
					blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blitRegion.srcSubresource.layerCount = 1;
					blitRegion.srcSubresource.mipLevel = 0;
					blitRegion.dstOffsets[0].x = (i % 2) * (halfW);
					blitRegion.dstOffsets[0].y = (i / 2) * halfH;
					blitRegion.dstOffsets[0].z = 1;
					blitRegion.dstOffsets[1].x = halfW + (i % 2) * halfW;
					blitRegion.dstOffsets[1].y = halfH + (i / 2) * halfH;
					blitRegion.dstOffsets[1].z = 1;
					blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blitRegion.dstSubresource.layerCount = 1;
					blitRegion.dstSubresource.mipLevel = 0;

					vkCmdBlitImage(cmd, this->multiPassImages[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								   this->getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
								   VK_FILTER_LINEAR);

					// TODO add sync.
				}
				/*	Blit the result to the default framebuffer.	*/

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}
		}

		virtual void update() {}
	};
	class MultiPassGLSample : public VKSample<SubPasses> {
	  public:
		MultiPassGLSample() : VKSample<SubPasses>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};

} // namespace vksample

int main(int argc, const char **argv) {
	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	try {
		vksample::MultiPassGLSample sample;

		sample.run(argc, argv, required_device_extensions, {}, required_instance_extensions);
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}