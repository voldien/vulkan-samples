#include "ImageImport.h"
#include "IOUtil.h"
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
										VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) {
		throw fragcore::RuntimeException("None Supported Image Format on Device: {}", magic_enum::enum_name(vk_format));
	}

	/*	Create staging buffer.	*/
	VKHelper::createImage(device, image.width(), image.height(), 1, vk_format, VK_IMAGE_TILING_LINEAR,
						  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProperties, textureImage, textureImageMemory);
	/*	*/
	VKHelper::transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VKHelper::copyBufferToImageCmd(
		cmd, stagingBuffer, textureImage,
		{static_cast<uint32_t>(image.width()), static_cast<uint32_t>(image.height()), image.layers()});

	/*	*/
	VKHelper::transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	/*	*/
	VKHelper::endSingleTimeCommands(device, queue, cmd, commandPool);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
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