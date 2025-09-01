#include "ImageImport.h"
#include "vulkan/vulkan_core.h"
#include <FreeImage.h>
#include <ImageFormat.h>
#include <Util/IOUtil.h>
#include <imageloader/ImageLoader.h>
#include <magic_enum.hpp>
#include <stdexcept>

using namespace fvkcore;
using namespace vksample;

// VkFormat image_format = VK_FORMAT_R8_SNORM;

// VkFormatProperties3 format_properties_3{};
// format_properties_3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR;

// // Properties3 need to be chained into Properties2
// VkFormatProperties2 format_properties_2{};
// format_properties_2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
// format_properties_2.pNext = &format_properties_3;

// // Get format properties for the select image format
// vkGetPhysicalDeviceFormatProperties2(this->getPhysicalDevice()->getHandle(), image_format,
// 									 &format_properties_2);
// if ((format_properties_3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT) == 0) {
// 	// Fallback to a different format or use other means of uploading data
// }

ImageImporter::ImageImporter(fragcore::IFileSystem *filesystem, VKSampleSessionBase &base)
	: filesystem(filesystem), base(base), device(*base.getVKDevice()) {}

void ImageImporter::loadTexture2D(const char *filename, Texture &texture, const ColorSpace colorSpace,
								  const TextureCompression compression, const void *pNext) {

	fragcore::ImageLoader imageLoader;
	fragcore::Image image = imageLoader.loadImage(filename);

	const size_t power_of_2 = std::floor(std::log(fragcore::Math::max(image.width(), image.height())) / std::log(2));
	size_t mipLevels = fragcore::Math::clamp<size_t>(power_of_2 - 4, 0, std::numeric_limits<size_t>::max());

	VkFormat vkImageFormat = VK_FORMAT_UNDEFINED;

	vkImageFormat = this->getImageFormat(image, colorSpace, compression);

	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkImageFormatProperties capabilityProperties = {};
	VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
	const VkImageType imageType = VK_IMAGE_TYPE_2D;

	/*	Check if combination supported.	*/
	if (!this->device.isFormatSupported(vkImageFormat, imageType, tiling, imageUsage, flags, &capabilityProperties)) {

		tiling = VK_IMAGE_TILING_LINEAR;

		if (!this->device.isFormatSupported(vkImageFormat, imageType, tiling, imageUsage, flags,
											&capabilityProperties)) {

			throw fragcore::RuntimeException("None Supported Image Format on Device: {}",
											 magic_enum::enum_name(vkImageFormat));
		}
	}
	mipLevels = fragcore::Math::clamp<size_t>(mipLevels, 1, capabilityProperties.maxMipLevels);

	/*	*/
	this->base.allocateImage(image.width(), image.height(), image.layers(), vkImageFormat, tiling, imageUsage,
							 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, flags, texture.image, texture.imageMemory);

	/*	*/
	this->base.transferImageData(texture.image, texture.imageMemory, image.width(), image.height(), image.layers(),
								 image.getPixelData(), image.getSize());

	(texture).width = image.width();
	(texture).height = image.height();
	(texture).depth = image.layers();
	texture.mipLevels = mipLevels;
	texture.internalformat = vkImageFormat;
	texture.tiling = tiling;

	if (texture.mipLevels > 1) {
		generateMipmaps(this->device.getHandle(), this->base.getTransferCommandPool(),
						this->base.getDefaultTransferQueue(), this->base.getPhysicalDevice()->getHandle(),
						texture.image, vkImageFormat, image.width(), image.height(), mipLevels);
	}
}

void ImageImporter::generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue,
									VkPhysicalDevice physicalDevice, VkImage image, VkFormat imageFormat,
									int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

	/*	Check if image format supports linear blitting	*/
	VkFormatProperties formatProperties;
	this->base.getPhysicalDevice()->getFormatProperties(imageFormat, formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = this->base.getTransferCommandBuffer();

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

	for (uint32_t blit_level = 1; blit_level < mipLevels; blit_level++) {

		barrier.subresourceRange.baseMipLevel = blit_level - 1;
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
		blit.srcSubresource.mipLevel = blit_level - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = blit_level;
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

		if (mipWidth > 1) {
			mipWidth /= 2;
		}
		if (mipHeight > 1) {
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
						 nullptr, 0, nullptr, 1, &barrier);

	this->base.endTransferCommand(commandBuffer);
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

void ImageImporter::saveTextureData(const char *cfilename, const Texture &texture) {

	void *pixelData = nullptr;
	unsigned int width = 0, height = 0, layers = 0;

	VkImageSubresource subResources = {};
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(base.getDevice(), texture.image, &subResources, &subResourceLayout);

	/*	Download texture data.	*/

	/*	Save data to texture.	*/
	this->saveTextureData(cfilename, pixelData, width, height, layers, 0);
}

void ImageImporter::saveTextureData(const char *cfilename, const void *pixelData, unsigned int width,
									unsigned int height, int layers, unsigned int format) {
	/*	*/
}

VkFormat ImageImporter::getImageFormat(fragcore::Image &image, const ColorSpace colorSpace,
									   const TextureCompression compression) {

	if (colorSpace == ColorSpace::SRGB) {
		switch (image.getFormat()) {
		case fragcore::ImageFormat::RGB24: /*	Multiple Channels.	*/
			return VK_FORMAT_R8G8B8_SRGB;
		case fragcore::ImageFormat::BGR24:
			return VK_FORMAT_B8G8R8_SRGB;
		case fragcore::ImageFormat::ARGB32:
			break;
		case fragcore::ImageFormat::BGRA32:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case fragcore::ImageFormat::RGBA32:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case fragcore::ImageFormat::RGBAFloat:
			break;
		case fragcore::ImageFormat::RGBFloat:

			break;
		case fragcore::ImageFormat::R8:
		case fragcore::ImageFormat::Alpha8: /*	Single Channel.	*/

			break;
		case fragcore::ImageFormat::RFloat:

			break;
		case fragcore::ImageFormat::R16:

			break;
		case fragcore::ImageFormat::R16U:

			break;
		case fragcore::ImageFormat::R32:

			break;
		case fragcore::ImageFormat::R32U:

			break;
		default:
			break;
		}
	}

	if (colorSpace == ColorSpace::RawLinear) {

		switch (image.getFormat()) {
		case fragcore::ImageFormat::RGB24:
			return VK_FORMAT_R8G8B8_UNORM;
		case fragcore::ImageFormat::RGBA32:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case fragcore::ImageFormat::BGR24:
			return VK_FORMAT_B8G8R8_UNORM;
		case fragcore::ImageFormat::BGRA32:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case fragcore::ImageFormat::RGBAFloat:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case fragcore::ImageFormat::RGBFloat:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case fragcore::ImageFormat::Alpha8: /*	Single Channel.	*/
		case fragcore::ImageFormat::RFloat:
		case fragcore::ImageFormat::R16:

		case fragcore::ImageFormat::R16U:

		case fragcore::ImageFormat::R32:
		case fragcore::ImageFormat::R32U:
		default:
			break;
		}
	}

	throw cxxexcept::RuntimeException("None Supported Format: {}", magic_enum::enum_name(image.getFormat()));
}
