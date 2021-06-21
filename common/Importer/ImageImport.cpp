#include "Importer/ImageImport.h"
#include "Importer/IOUtil.h"
#include <FreeImage.h>
#include <stdexcept>

void *ImageImporter::loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
									 unsigned int *pformat, unsigned int *pinternalformat, unsigned int *ptype,
									 unsigned long *pixelSize) {

	/*	Free image.	*/
	FREE_IMAGE_FORMAT imgtype;		 /**/
	FREE_IMAGE_COLOR_TYPE colortype; /**/
	FREE_IMAGE_TYPE imgt;			 /**/
	FIMEMORY *stream;				 /**/
	FIBITMAP *firsbitmap;			 /**/
	void *pixel;					 /**/
	unsigned int bpp;

	std::vector<char> io = IOUtil::readFile(cfilename);

	if (io.size() <= 0)
		throw std::invalid_argument("Texture data must be greater than 0.");

	/*	1 byte for the size in order, Because it crash otherwise if set to 0.	*/
	stream = FreeImage_OpenMemory((BYTE *)io.data(), io.size());
	if (stream == nullptr)
		throw std::runtime_error(fmt::format("Failed to open freeimage memory stream. \n"));

	/*	Seek to beginning of the memory stream.	*/
	FreeImage_SeekMemory(stream, 0, SEEK_SET);

	/*	Load image from */
	imgtype = FreeImage_GetFileTypeFromMemory(stream, io.size());
	FreeImage_SeekMemory(stream, 0, SEEK_SET);
	firsbitmap = FreeImage_LoadFromMemory(imgtype, stream, 0);
	if (firsbitmap == nullptr) {
		FreeImage_CloseMemory(stream);
		throw std::runtime_error(fmt::format("Failed to create free-image from memory.\n"));
	}

	/*	Reset to beginning of stream.	*/
	FreeImage_SeekMemory(stream, 0, SEEK_SET);
	imgt = FreeImage_GetImageType(firsbitmap);
	colortype = FreeImage_GetColorType(firsbitmap);
	// imagetype = FreeImage_GetImageType(firsbitmap);

	switch (colortype) {
	case FIC_RGB:
		if (pformat)
			*pformat = (Format)TextureFormat::BGR24; // eBGR;
		if (pinternalformat)
			*pinternalformat = (Format)GraphicFormat::R8G8B8_SRGB; // eRGB;
		bpp = 3;
		break;
	case FIC_RGBALPHA:
		if (pformat)
			*pformat = (Format)TextureFormat::BGRA32; // eBGR;
		if (pinternalformat)
			*pinternalformat = (Format)GraphicFormat::R8G8B8A8_SRGB; // eRGB;
		/*			if (pformat)
						*pformat = eBGRA;
					if (pinternalformat)
						*pinternalformat = eRGBA;*/
		bpp = 4;
		break;
	case FIC_MINISWHITE:
	case FIC_MINISBLACK:
		if (pformat)
			*pformat = SingleColor;
		if (pinternalformat)
			*pinternalformat = SingleColor;
		bpp = 1;
		break;
	default:
		break;
	}

	/*	Get attributes from the image.	*/
	pixel = FreeImage_GetBits(firsbitmap);
	if (pwidth)
		*pwidth = FreeImage_GetWidth(firsbitmap);
	if (pheight)
		*pheight = FreeImage_GetHeight(firsbitmap);
	bpp = (FreeImage_GetBPP(firsbitmap) / 8);
	if (pixelSize)
		*pixelSize = (*pwidth) * (*pheight) * bpp;

	if (ptype)
		*ptype = UnsignedByte;

	/*	Check error and release resources.	*/
	if (pixel == nullptr || io.size() == 0) {
		FreeImage_Unload(firsbitmap);
		FreeImage_CloseMemory(stream);
		throw std::runtime_error(fmt::format("Failed getting pixel data from FreeImage.\n"));
	}

	/*	Make a copy of pixel data.	*/
	void *pixels = malloc(*pixelSize);
	if (pixels == nullptr)
		throw std::runtime_error(fmt::format("Failed to allocate %d, %s.\n", io.size(), strerror(errno)));

	memcpy(pixels, pixel, *pixelSize);

	/*	Release free image resources.	*/
	FreeImage_Unload(firsbitmap);
	FreeImage_CloseMemory(stream);
	return pixels;
}

void ImageImporter::saveTextureData(const char *cfilename, const void *pixelData, unsigned int width,
									unsigned int height, int layers, unsigned int format) {}

void ImageImporter::saveTextureData(const char* cfilename, VkDevice device, VkImage image){

	void* pixelData;
	unsigned int width, height, layers;

	VkImageSubresource subResources = {};
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(device, image, &subResources, &subResourceLayout);

	/*	Download texture data.	*/
	


	/*	Save data to texture.	*/
	saveTextureData(cfilename, pixelData, width, height, layers, 0);
}


void ImageImporter::createImage(const char *filename, VkDevice device, VkCommandBuffer cmd,
								VkPhysicalDevice physicalDevice, VkImage &textureImage,
								VkDeviceMemory &textureImageMemory) {

	unsigned int texWidth, texHeight, internal, type, format;
	unsigned long pixelSize;
	void *pixels = loadTextureData(filename, &texWidth, &texHeight, &format, &internal, &type, &pixelSize);

	VkDeviceSize imageSize = pixelSize;
	VkPhysicalDeviceMemoryProperties memProperties;

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	/*	*/
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VKHelper::createBuffer(device, imageSize, memProperties, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
						   stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	VKHelper::createImage(device, texWidth, texHeight, 1, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
						  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProperties, textureImage, textureImageMemory);

	VKHelper::transitionImageLayout(cmd, textureImage, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
									VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VKHelper::copyBufferToImageCmd(device, cmd, stagingBuffer, textureImage,
								   {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1});

	VKHelper::transitionImageLayout(cmd, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// vkDestroyBuffer(device, stagingBuffer, nullptr);
	// vkFreeMemory(device, stagingBufferMemory, nullptr);

	free(pixels);
}