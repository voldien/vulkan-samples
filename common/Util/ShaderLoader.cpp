#include "ShaderLoader.h"
#include "PipelineLayoutUtil.h"
#include "VKHelper.h"
#include "VKSampleBase.h"
#include "vulkan/vulkan_core.h"
#include <VulkanCore.h>

using namespace vksample;
using namespace fvkcore;

ShaderLoader::ShaderLoader(VKSampleSessionBase &renderBackend)
	: renderer(renderBackend), pipelineCache(renderBackend.getPipelineCache()) {}

static void mergeDescriptorBinding(const std::vector<VkDescriptorSetLayoutBinding> &bindings,
								   std::vector<VkDescriptorSetLayoutBinding> &mergedBindings) {

	for (size_t i = 0; i < bindings.size(); i++) {
		const VkDescriptorSetLayoutBinding &bind = bindings[i];
		bool exists = false;

		for (size_t j = 0; j < mergedBindings.size(); j++) {
			VkDescriptorSetLayoutBinding &merge = mergedBindings[j];

			/*	Exists.	*/
			if (merge.binding == bind.binding && merge.descriptorCount == bind.descriptorCount &&
				merge.descriptorType == bind.descriptorType) {

				merge.stageFlags |= bind.stageFlags;
				exists = true;
			}
		}

		if (!exists) {
			mergedBindings.push_back(bind);
		}
	}
}

// TODO: create all the descriptor set laypout

GraphicPipeline *ShaderLoader::loadGraphicProgram(const std::vector<uint32_t> *vertex,
												  const std::vector<uint32_t> *fragment,
												  const std::vector<uint32_t> *geometry,
												  const std::vector<uint32_t> *tesselationc,
												  const std::vector<uint32_t> *tesselatione, const void *pNext) {

	GraphicPipeline *graphicPipeline = new GraphicPipeline();
	VkDevice device = this->renderer.getDevice();

	VkShaderModule vertShaderModule = VK_NULL_HANDLE;
	VkShaderModule fragShaderModule = VK_NULL_HANDLE;
	VkShaderModule geoShaderModule = VK_NULL_HANDLE;
	VkShaderModule tescShaderModule = VK_NULL_HANDLE;
	VkShaderModule teseShaderModule = VK_NULL_HANDLE;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {};

	if (vertex) {
		vertShaderModule = VKHelper::createShaderModule<uint32_t>(device, *vertex);
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		shaderStages.push_back(vertShaderStageInfo);
	}
	if (fragment) {
		fragShaderModule = VKHelper::createShaderModule(device, *fragment);
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		shaderStages.push_back(fragShaderStageInfo);
	}

	if (geometry) {
		geoShaderModule = VKHelper::createShaderModule(device, *geometry);
		VkPipelineShaderStageCreateInfo geoShaderStageInfo{};
		geoShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geoShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geoShaderStageInfo.module = geoShaderModule;
		geoShaderStageInfo.pName = "main";
		shaderStages.push_back(geoShaderStageInfo);
	}
	if (tesselationc) {
		tescShaderModule = VKHelper::createShaderModule(device, *tesselationc);
		VkPipelineShaderStageCreateInfo geoShaderStageInfo{};
		geoShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geoShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		geoShaderStageInfo.module = geoShaderModule;
		geoShaderStageInfo.pName = "main";
		shaderStages.push_back(geoShaderStageInfo);
	}
	if (tesselatione) {
		teseShaderModule = VKHelper::createShaderModule(device, *tesselatione);
		VkPipelineShaderStageCreateInfo geoShaderStageInfo{};
		geoShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geoShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		geoShaderStageInfo.module = geoShaderModule;
		geoShaderStageInfo.pName = "main";
		shaderStages.push_back(geoShaderStageInfo);
	}

	const PipelineLayoutUtil::InputLayout input_layout = PipelineLayoutUtil::getInputLayout(*vertex);

	std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> bindingLayouts(3);

	if (vertex) {
		const std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> layoutBindingVertex =
			PipelineLayoutUtil::getDefaultBinding(*vertex);

		for (size_t i = 0; i < layoutBindingVertex.size(); i++) {
			bindingLayouts[i].bindings.insert(bindingLayouts[i].bindings.end(), layoutBindingVertex[i].bindings.begin(),
											  layoutBindingVertex[i].bindings.end());
		}
	}
	if (fragment) {
		const std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> layoutBindingFragment =
			PipelineLayoutUtil::getDefaultBinding(*fragment);

		for (size_t i = 0; i < layoutBindingFragment.size(); i++) {
			bindingLayouts[i].bindings.insert(bindingLayouts[i].bindings.end(),
											  layoutBindingFragment[i].bindings.begin(),
											  layoutBindingFragment[i].bindings.end());
		}
	}
	if (geometry) {
		const std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> layoutBindingGeometry =
			PipelineLayoutUtil::getDefaultBinding(*geometry);
	}
	if (tesselationc) {
		const std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> layoutBindingTesselationC =
			PipelineLayoutUtil::getDefaultBinding(*tesselationc);
	}
	if (tesselatione) {
		const std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> layoutBindingTesselationE =
			PipelineLayoutUtil::getDefaultBinding(*tesselatione);
	}

	// // TODO: Remapping of stuff.
	// std::vector<PipelineLayoutUtil::DescriptorSetLayoutData> layoutSet0 = layoutBindingVertex;
	// layoutSet0.insert(layoutSet0.end(), layoutBindingFragment.begin(), layoutBindingFragment.end());

	// std::vector<VkDescriptorSetLayoutBinding> binding_layout = layoutBindingVertex[0].bindings;
	// // for (size_t i = 0; i < layout0.size(); i++) {
	// binding_layout.insert(binding_layout.end(), layoutBindingFragment[0].bindings.begin(),
	// 					  layoutBindingFragment[0].bindings.end());
	//	}
	for (size_t i = 0; i < bindingLayouts.size(); i++) {
		std::vector<VkDescriptorSetLayoutBinding> finalLayout;
		mergeDescriptorBinding(bindingLayouts[i].bindings, finalLayout);

		// VK_DESCRIPTOR_SET_LAYOUT_CREATE_PER_STAGE_BIT_NV;
		const VkDescriptorSetLayoutCreateFlags flags = 0; // VK_DESCRIPTOR_SET_LAYOUT_CREATE_PER_STAGE_BIT_NV;
		VKHelper::createDescriptorSetLayout(device, graphicPipeline->setLayout[i], finalLayout, flags,
											renderer.getAllocatorCallback());
	}
	graphicPipeline->numSetLayout = bindingLayouts.size();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = 1; // sizeof(fragcore::ProceduralGeometry::Vertex); // sizeof(float) * (3 + 2 + 3 + 3);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputInfo.vertexBindingDescriptionCount = 1;

	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(input_layout.inputAttributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = input_layout.inputAttributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(1);
	viewport.height = static_cast<float>(1);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent.width = 1;
	scissor.extent.height = 1;

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

	const std::vector<VkDescriptorSetLayout> descriptor_layout(
		graphicPipeline->setLayout.begin(), graphicPipeline->setLayout.begin() + graphicPipeline->numSetLayout);

	VkPipelineLayoutCreateFlags pipeline_flags = 0;
	VKHelper::createPipelineLayout(device, graphicPipeline->layout, pipeline_flags, descriptor_layout, {},
								   renderer.getAllocatorCallback());

	/*  */
	std::vector<VkDynamicState> dynamicStateEnables(2);
	dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStateEnables[1] = VK_DYNAMIC_STATE_SCISSOR;

	const bool hasDynamicState{false};
	const bool hasDynamicState2{false};
	const bool hasDynamicState3{false};

	if (hasDynamicState) {
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_CULL_MODE_EXT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_FRONT_FACE_EXT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT);
	}
	if (hasDynamicState2) {
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT);
	}
	if (hasDynamicState3) {
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT);
	}

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.pNext = nullptr;
	dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
	dynamicStateInfo.dynamicStateCount = dynamicStateEnables.size();

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	const VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.pNext = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &format,
	};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = graphicPipeline->layout;
	pipelineInfo.renderPass =
		nullptr; // this->renderer.getDef engine->defaultFrameBuffer.renderpass; // TODO: fix reference
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDynamicState = &dynamicStateInfo;

	VKS_VALIDATE(vkCreateGraphicsPipelines(device, this->pipelineCache, 1, &pipelineInfo,
										   this->renderer.getAllocatorCallback(), &graphicPipeline->pipeline));

	// TODO: destroy with for loop.
	for (size_t i = 0; i < shaderStages.size(); i++) {
		vkDestroyShaderModule(device, shaderStages[i].module, this->renderer.getAllocatorCallback());
	}

	return graphicPipeline;
}

ComputePipeline ShaderLoader::loadComputeProgram(const std::vector<uint32_t> *computeBinary, const void *pNext) {
	ComputePipeline computePipe;

	VkDevice device = this->renderer.getDevice();

	VkShaderModule compShaderModule = VKHelper::createShaderModule(device, *computeBinary);

	VkPipelineShaderStageCreateInfo compShaderStageInfo{};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";

	const auto layoutBinding = PipelineLayoutUtil::getDefaultBinding(*computeBinary);

	computePipe.numSetLayout = layoutBinding.size();

	/*	*/
	const VkDescriptorSetLayoutCreateFlags flags = 0;
	VKHelper::createDescriptorSetLayout(device, computePipe.setLayout[0], layoutBinding[0].bindings, flags,
										renderer.getAllocatorCallback());

	/*	*/

	VkPipelineLayoutCreateFlags pipeline_flags = 0;
	VKHelper::createPipelineLayout(device, computePipe.layout, pipeline_flags, {computePipe.setLayout[0]});

	computePipe.pipeline = VKHelper::createComputePipeline(device, computePipe.layout, compShaderStageInfo);

	vkDestroyShaderModule(device, compShaderModule, nullptr);

	return computePipe;
}

RayTracingPipeline ShaderLoader::loadRayTracingProgram(const std::vector<uint32_t> *anyHitBinary,
													   const std::vector<uint32_t> *closestHitBinary,
													   const std::vector<uint32_t> *generalHitBinary,
													   const std::vector<uint32_t> *genIntersectionBinary,
													   const std::vector<uint32_t> *genRayBinary,
													   const std::vector<uint32_t> *genMissBinary) {

	VkDevice device = this->renderer.getDevice();

	/*	*/
	RayTracingPipeline pipeline;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {};

	VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};

	VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
	// std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;

	// Assemble the shader stages and recursion depth info into the ray tracing pipeline
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
	rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size()); // Stages are shaders
	rayPipelineInfo.pStages = shaderStages.data();

	// In this case, m_rtShaderGroups.size() == 4: we have one raygen group,
	// two miss shader groups, and one hit group.
	// rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
	// rayPipelineInfo.pGroups = m_rtShaderGroups.data();

	// The ray tracing process can shoot rays from the camera, and a shadow ray can be shot from the
	// hit points of the camera rays, hence a recursion level of 2. This number should be kept as low
	// as possible for performance reasons. Even recursive ray tracing should be flattened into a loop
	// in the ray generation to avoid deep recursion.
	rayPipelineInfo.maxPipelineRayRecursionDepth = 2; // Ray depth
	rayPipelineInfo.layout = pipeline.layout;

	// vkCreateRayTracingPipelinesKHR(device, {}, {}, 1, &rayPipelineInfo, nullptr, &pipeline.pipeline);

	return pipeline;
}