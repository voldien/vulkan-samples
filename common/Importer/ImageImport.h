#ifndef _VK_SAMPLE_IMAGE_IMPORTER_H_
#define _VK_SAMPLE_IMAGE_IMPORTER_H_ 1
#include "../Core/VKDevice.h"
#include "../Core/VKHelper.h"
#include <stdio.h>
#include <vulkan/vulkan.h>

/**
 * @brief
 *
 */
class ImageImporter {
  public:
  public:
	static void *loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
								 unsigned int *pformat, unsigned int *pinternalformat, unsigned int *ptype,
								 unsigned long *pixelSize);
	static void saveTextureData(const char *cfilename, const void *pixelData, unsigned int width, unsigned int height,
								int layers, unsigned int format);

	/**/
	static void saveTextureData(const char *filename, VkDevice device, VkImage image);

	static void createImage(const char *filename, const VKDevice &device, VkImage &image);

	/*	*/
	static void createImage2D(const char *filename, VkDevice device, VkCommandPool commandPool, VkQueue queue,
							  VkPhysicalDevice physicalDevice, VkImage &textureImage,
							  VkDeviceMemory &textureImageMemory);

	static void createCubeMap(const std::vector<std::string> &paths, VkDevice device, VkCommandPool commandPool,
							  VkQueue queue, VkPhysicalDevice physicalDevice, VkImage &textureImage,
							  VkDeviceMemory &textureImageMemory);
};

#endif
