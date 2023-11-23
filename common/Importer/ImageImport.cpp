#include "ImageImport.h"
#include "IOUtil.h"
#include "VKHelper.h"
#include <FreeImage.h>
#include <ImageFormat.h>
#include <imageloader/ImageLoader.h>
#include <magic_enum.hpp>
#include <stdexcept>

using namespace fvkcore;
using namespace vksample;

ImageImporter::ImageImporter(fragcore::IFileSystem *filesystem, VKDevice &device)
	: filesystem(filesystem), device(device) {}

void ImageImporter::saveTextureData(const char *cfilename, const void *pixelData, unsigned int width,
									unsigned int height, int layers, unsigned int format) {}

void ImageImporter::saveTextureData(const char *cfilename, VkDevice device, VkImage image) {

	void *pixelData;
	unsigned int width, height, layers;

	VkImageSubresource subResources = {};
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(device, image, &subResources, &subResourceLayout);

	/*	Download texture data.	*/

	// Seperate thread.
	/*	Save data to texture.	*/
	saveTextureData(cfilename, pixelData, width, height, layers, 0);
}

void ImageImporter::createImage2D(const char *filename, VkDevice device, VkCommandPool commandPool, VkQueue queue,
								  VkPhysicalDevice physicalDevice, VkImage &textureImage,
								  VkDeviceMemory &textureImageMemory) {

	fragcore::ImageLoader imageLoader;
	fragcore::Image image = imageLoader.loadImage(filename);

	uint32_t mipLevels = std::min(
		static_cast<uint32_t>(std::floor(std::log2(std::max(image.width(), image.height())))) + 1, (uint32_t)8);

	const VkDeviceSize imageSize = image.getSize();
	VkPhysicalDeviceMemoryProperties memProperties;

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	/*	*/
	VkCommandBuffer cmd = VKHelper::beginSingleTimeCommands(device, commandPool);

	/*	*/
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VKHelper::createBuffer(device, imageSize, memProperties, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer, stagingBufferMemory);

	/*	Write image data.	*/
	void *stageData;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &stageData);
	memcpy(stageData, image.getPixelData(), static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	VkFormat vk_format;
	switch (image.getFormat()) {
	case fragcore::TextureFormat::RGB24:
		vk_format = VK_FORMAT_R8G8B8_UNORM;
		break;
	case fragcore::TextureFormat::RGBA32:
		vk_format = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case fragcore::TextureFormat::BGR24:
		vk_format = VK_FORMAT_B8G8R8_UNORM;
		break;
	case fragcore::TextureFormat::BGRA32:
		vk_format = VK_FORMAT_B8G8R8A8_UNORM;
		break;
	case fragcore::TextureFormat::RGBAFloat:
		vk_format = VK_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case fragcore::TextureFormat::RGBFloat:
		vk_format = VK_FORMAT_R32G32B32_SFLOAT;
		break;
	default:
		throw fragcore::RuntimeException("None Supported Format: {}", magic_enum::enum_name(image.getFormat()));
		break;
	}

	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	// TODO fix VK_IMAGE_TILING_LINEAR or tiling
	/*	TODO check if combination supported.	*/
	if (!this->device.isFormatSupported(vk_format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR,
										VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
											VK_IMAGE_USAGE_SAMPLED_BIT)) {
		throw fragcore::RuntimeException("None Supported Image Format on Device: {}", magic_enum::enum_name(vk_format));
	}

	/*	Create staging buffer.	*/
	VKHelper::createImage(device, image.width(), image.height(), mipLevels, vk_format, VK_IMAGE_TILING_LINEAR,
						  VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
							  VK_IMAGE_USAGE_SAMPLED_BIT,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProperties, textureImage, textureImageMemory);
	/*	*/
	VKHelper::transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VKHelper::copyBufferToImageCmd(
		cmd, stagingBuffer, textureImage,
		{static_cast<uint32_t>(image.width()), static_cast<uint32_t>(image.height()), image.layers()});

	/*	*/
	VKHelper::endSingleTimeCommands(device, queue, cmd, commandPool);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	generateMipmaps(device, commandPool, queue, physicalDevice, textureImage, vk_format, image.width(), image.height(),
					mipLevels);
}

void ImageImporter::generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue,
									VkPhysicalDevice physicalDevice, VkImage image, VkFormat imageFormat,
									int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = VKHelper::beginSingleTimeCommands(device, commandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
							 nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
							 nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
						 nullptr, 0, nullptr, 1, &barrier);

	VKHelper::endSingleTimeCommands(device, queue, commandBuffer, commandPool);
}

void ImageImporter::createCubeMap(const std::vector<std::string> &paths, VkDevice device, VkCommandPool commandPool,
								  VkQueue queue, VkPhysicalDevice physicalDevice, VkImage &textureImage,
								  VkDeviceMemory &textureImageMemory) {
	// ImageLoader imageLoader;
	// VkBuffer stagingBuffer;
	// VkDeviceMemory stagingBufferMemory;
	// VkFormat vk_format;
	// VkCommandBuffer cmd = VKHelper::beginSingleTimeCommands(device, commandPool);

	// int width, height;

	// /*	Create staging buffer.	*/
	// VKHelper::createImage(device, width, height, 6, vk_format, VK_IMAGE_TILING_OPTIMAL,
	// 					  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	// 					  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProperties, textureImage, textureImageMemory);

	// for (size_t i = 0; i < paths.size(); i++) {
	// 	Image image = imageLoader.loadImage(paths[i]);

	// 	const VkDeviceSize imageSize = image.getSize();
	// 	VkPhysicalDeviceMemoryProperties memProperties;

	// 	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// 	/*	*/
	// 	VKHelper::transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_UNDEFINED,
	// 									VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// 	VKHelper::copyBufferToImageCmd(
	// 		cmd, stagingBuffer, textureImage,
	// 		{static_cast<uint32_t>(image.width()), static_cast<uint32_t>(image.height()), image.layers()});

	// 	/*	*/
	// 	VKHelper::transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	// 									VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	// }
	// /*	*/
	// VKHelper::endSingleTimeCommands(device, queue, cmd, commandPool);
	// vkDestroyBuffer(device, stagingBuffer, nullptr);
	// vkFreeMemory(device, stagingBufferMemory, nullptr);
}