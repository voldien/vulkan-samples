#pragma once
#include <Core/IO/IFileSystem.h>
#include <FragDef.h>
#include <VKDevice.h>
#include <VKHelper.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC ImageImporter {
	  public:
		ImageImporter(fragcore::IFileSystem *filesystem, fvkcore::VKDevice &device);

	  public:
		static void *loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
									 unsigned int *pformat, unsigned int *pinternalformat, unsigned int *ptype,
									 unsigned long *pixelSize);
		static void saveTextureData(const char *cfilename, const void *pixelData, unsigned int width,
									unsigned int height, int layers, unsigned int format);

		/**/
		static void saveTextureData(const char *filename, VkDevice device, VkImage image);

		static void createImage(const char *filename, const VkDevice &device, VkImage &image);

		/*	*/
		void createImage2D(const char *filename, VkDevice device, VkCommandPool commandPool, VkQueue queue,
						   VkPhysicalDevice physicalDevice, VkImage &textureImage, VkDeviceMemory &textureImageMemory);

		static void createCubeMap(const std::vector<std::string> &paths, VkDevice device, VkCommandPool commandPool,
								  VkQueue queue, VkPhysicalDevice physicalDevice, VkImage &textureImage,
								  VkDeviceMemory &textureImageMemory);

		void generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue,
						   VkPhysicalDevice physicalDevice, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
							 uint32_t mipLevels);

	  private:
		fragcore::IFileSystem *filesystem;
		fvkcore::VKDevice &device;
	};

} // namespace vksample