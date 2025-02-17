#pragma once
#include <FragDef.h>
#include <IO/IFileSystem.h>
#include <VKDevice.h>
#include <VKHelper.h>
#include <vulkan/vulkan.h>

namespace vksample {

	enum class TextureCompression {
		None,	 /*	*/
		Default, /*	*/
		BPTC,
		ACST
	};

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC ImageImporter {
	  public:
		ImageImporter(fragcore::IFileSystem *filesystem, fvkcore::VKDevice &device);
		virtual ~ImageImporter() = default;

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
		void loadImage2D(const char *filename, VkDevice device, VkCommandPool commandPool, VkQueue queue,
						 VkPhysicalDevice physicalDevice, VkImage &textureImage, VkDeviceMemory &textureImageMemory);
		// int loadImage2DRaw(const fragcore::Image &image, const ColorSpace colorSpace = ColorSpace::RawLinear,
		// 				   const TextureCompression compression = TextureCompression::None);

		static void createCubeMap(const std::vector<std::string> &paths, VkDevice device, VkCommandPool commandPool,
								  VkQueue queue, VkPhysicalDevice physicalDevice, VkImage &textureImage,
								  VkDeviceMemory &textureImageMemory);

		// int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
		// 				const std::string &pz, const std::string &nz, const ColorSpace colorSpace =
		// ColorSpace::RawLinear, 				const TextureCompression compression = TextureCompression::None); int
		// loadCubeMap(const std::vector<std::string> &paths, const ColorSpace colorSpace = ColorSpace::RawLinear,
		// const TextureCompression compression = TextureCompression::None);

		void generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkPhysicalDevice physicalDevice,
							 VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
							 uint32_t mipLevels);

	  private:
		fragcore::IFileSystem *filesystem;
		fvkcore::VKDevice &device;
	};

} // namespace vksample