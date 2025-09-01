#pragma once
#include "Image.h"
#include "VKDataStructure.h"
#include "VKSampleBase.h"
#include <FragDef.h>
#include <IO/IFileSystem.h>
#include <VKDevice.h>
#include <VKHelper.h>
#include <vulkan/vulkan.h>

namespace vksample {

	enum class ColorSpace : unsigned int {
		RawLinear = 0,	   /*	Linear.	*/
		SRGB,			   /*	SRGB encoded.	*/
		ACES,			   /*	*/
		Filmic,			   /*	*/
		KhronosPBRNeutral, /*	*/
		FalseColor,		   /*	*/
		MaxColorSpaces
	};

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
		ImageImporter(fragcore::IFileSystem *filesystem, VKSampleSessionBase &base); // TODO: change to base sample
		virtual ~ImageImporter() = default;

	  public:
		void loadTexture2D(const char *filename, Texture &texture, const ColorSpace colorSpace = ColorSpace::RawLinear,
						   const TextureCompression compression = TextureCompression::None,
						   const void *pNext = nullptr);

		void loadTexture2DAsync(const char *filename, Texture **texture,
								const ColorSpace colorSpace = ColorSpace::RawLinear,
								const TextureCompression compression = TextureCompression::None,
								const void *pNext = nullptr);

		// int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
		// 				const std::string &pz, const std::string &nz, const ColorSpace colorSpace =
		// ColorSpace::RawLinear, 				const TextureCompression compression = TextureCompression::None); int
		// loadCubeMap(const std::vector<std::string> &paths, const ColorSpace colorSpace = ColorSpace::RawLinear,
		// const TextureCompression compression = TextureCompression::None);

		static void createCubeMap(const std::vector<std::string> &paths, VkDevice device, VkCommandPool commandPool,
								  VkQueue queue, VkPhysicalDevice physicalDevice, VkImage &textureImage,
								  VkDeviceMemory &textureImageMemory);

		// static void *loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
		// 							 unsigned int *pformat, unsigned int *pinternalformat, unsigned int *ptype,
		// 							 unsigned long *pixelSize);

		/**/
		void saveTextureData(const char *filename, const Texture &texture);

		void saveTextureData(const char *cfilename, const void *pixelData, unsigned int width, unsigned int height,
							 int layers, unsigned int format);

	  protected:
		VkFormat getImageFormat(fragcore::Image &image, const ColorSpace colorSpace,
								const TextureCompression compression);

		void generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkPhysicalDevice physicalDevice,
							 VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
							 uint32_t mipLevels);

	  private:
		fragcore::IFileSystem *filesystem;
		fvkcore::VKDevice &device;
		VKSampleSessionBase &base;
	};

} // namespace vksample
