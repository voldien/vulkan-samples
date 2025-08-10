#pragma once
#include "vulkan/vulkan_core.h"
#include <array>
#include <glm/ext/vector_int3.hpp>

// TODO: rename file

namespace vksample {

	class Texture {
	  public:
		VkImage image;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkFormat format;
		VkImageTiling tiling;
		unsigned int width;
		unsigned int height;
		unsigned int depth;
		unsigned int mipLevels;
	};

	class UBOObject {
	  public:
		VkBuffer buffer; /*	*/
		VkDeviceMemory memory;
		size_t size;			/*	*/
		size_t totalSize;		/*	*/
		unsigned int alignment; /*	*/
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
} // namespace vksample
