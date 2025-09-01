/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Core/Object.h"
#include "DataStructure/MemoryAddress.h"
#include "vulkan/vulkan_core.h"
#include <array>
#include <glm/ext/vector_int3.hpp>

// TODO: rename file

namespace vksample {

	class Texture {
	  public:
		VkImage image{nullptr};
		VkDeviceMemory imageMemory{nullptr};
		VkImageView imageView{nullptr};
		VkFormat internalformat;
		VkImageTiling tiling;
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int depth = 0;
		unsigned int mipLevels = 0;
	};

	class UBOObject {
	  public:
		VkBuffer buffer; /*	*/
		VkDeviceMemory memory;
		size_t size;			/*	*/
		size_t totalSize;		/*	*/
		unsigned int alignment; /*	*/
	};

	using UBOPool = struct uniform_buffer_pool_object_t {
		UBOObject buffer{};
		fragcore::MemoryAddress addresser;
	};

	using FrameBuffer = struct framebuffer_t {
		VkFramebuffer framebuffer{};
		VkRenderPass renderpass{};

		std::array<Texture, 16> attachments{}; /*	last */
		// std::array<VkAttachmentDescription, 16> attachements__{};
		std::array<glm::ivec3, 16> attachmentSize{};
		unsigned int nrAttachments = 0;
		unsigned int depthIndex = 15;
	};

	using MeshObject = struct geometry_object_t {
		/*	*/
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkBuffer indicesBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
		VkDeviceMemory indicesMemory = VK_NULL_HANDLE;

		size_t nrIndicesElements = 0;
		size_t nrVertices = 0;

		size_t vertex_offset = 0;
		size_t indices_offset = 0;

		unsigned int stride = 0;
		int primitiveType = 0;

		/*	*/
		// fragcore::Bound bound{};
	};

	//: public fragcore::Object
	class ShaderPipeline {
	  public:
		VkPipeline pipeline;	 /*	*/
		VkPipelineLayout layout; /*	*/

		std::array<VkDescriptorSetLayout, 8> setLayout; /*	*/
		uint32_t numSetLayout;

		VkDescriptorSet set; /*	*/
	};

	class GraphicPipeline : public ShaderPipeline {
	  public:
		GraphicPipeline() = default;
	};

	class ComputePipeline : public ShaderPipeline {
	  public:
		ComputePipeline() = default;
	};

	class RayTracingPipeline : public ShaderPipeline {
	  public:
		RayTracingPipeline() = default;
	};
} // namespace vksample
