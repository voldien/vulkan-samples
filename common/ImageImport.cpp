// #include"ImageImport.h"
// #include <FreeImage.h>
// #include <stdexcept>

// void *ImageImport::loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
// 							 TextureDesc::Format *pformat, TextureDesc::Format *pinternalformat,
// 							 TextureDesc::Type *ptype, unsigned long *pixelSize);

// /*	Free image.	*/
// FREE_IMAGE_FORMAT imgtype;		 /**/
// FREE_IMAGE_COLOR_TYPE colortype; /**/
// FREE_IMAGE_TYPE imgt;			 /**/
// FIMEMORY *stream;				 /**/
// FIBITMAP *firsbitmap;			 /**/
// void *pixel;					 /**/
// unsigned int bpp;

// if (size <= 0)
// 	throw std::invalid_argument("Texture data must be greater than 0.");

// /*	1 byte for the size in order, Because it crash otherwise if set to 0.	*/
// stream = FreeImage_OpenMemory((BYTE *)pbuf, size);
// if (stream == nullptr)
// 	throw RuntimeException(fvformatf("Failed to open freeimage memory stream. \n"));

// /*	Seek to beginning of the memory stream.	*/
// FreeImage_SeekMemory(stream, 0, SEEK_SET);

// /*	Load image from */
// imgtype = FreeImage_GetFileTypeFromMemory(stream, size);
// FreeImage_SeekMemory(stream, 0, SEEK_SET);
// firsbitmap = FreeImage_LoadFromMemory(imgtype, stream, 0);
// if (firsbitmap == nullptr) {
// 	FreeImage_CloseMemory(stream);
// 	throw RuntimeException(fvformatf("Failed to create free-image from memory.\n"));
// }

// /*	Reset to beginning of stream.	*/
// FreeImage_SeekMemory(stream, 0, SEEK_SET);
// imgt = FreeImage_GetImageType(firsbitmap);
// colortype = FreeImage_GetColorType(firsbitmap);
// // imagetype = FreeImage_GetImageType(firsbitmap);

// switch (colortype) {
// case FIC_RGB:
// 	if (pformat)
// 		*pformat = (TextureDesc::Format)TextureFormat::BGR24; // TextureDesc::eBGR;
// 	if (pinternalformat)
// 		*pinternalformat = (TextureDesc::Format)GraphicFormat::R8G8B8_SRGB; // TextureDesc::eRGB;
// 	bpp = 3;
// 	break;
// case FIC_RGBALPHA:
// 	if (pformat)
// 		*pformat = (TextureDesc::Format)TextureFormat::BGRA32; // TextureDesc::eBGR;
// 	if (pinternalformat)
// 		*pinternalformat = (TextureDesc::Format)GraphicFormat::R8G8B8A8_SRGB; // TextureDesc::eRGB;
// 	/*			if (pformat)
// 					*pformat = TextureDesc::eBGRA;
// 				if (pinternalformat)
// 					*pinternalformat = TextureDesc::eRGBA;*/
// 	bpp = 4;
// 	break;
// case FIC_MINISWHITE:
// case FIC_MINISBLACK:
// 	if (pformat)
// 		*pformat = TextureDesc::eSingleColor;
// 	if (pinternalformat)
// 		*pinternalformat = TextureDesc::eSingleColor;
// 	bpp = 1;
// 	break;
// default:
// 	break;
// }

// /*	Get attributes from the image.	*/
// pixel = FreeImage_GetBits(firsbitmap);
// if (width)
// 	*width = FreeImage_GetWidth(firsbitmap);
// if (height)
// 	*height = FreeImage_GetHeight(firsbitmap);
// bpp = (FreeImage_GetBPP(firsbitmap) / 8);
// if (pixelSize)
// 	*pixelSize = (*width) * (*height) * bpp;

// if (ptype)
// 	*ptype = TextureDesc::eUnsignedByte;

// /*	Check error and release resources.	*/
// if (pixel == nullptr || size == 0) {
// 	FreeImage_Unload(firsbitmap);
// 	FreeImage_CloseMemory(stream);
// 	throw RuntimeException(fvformatf("Failed getting pixel data from FreeImage.\n"));
// }

// /*	Make a copy of pixel data.	*/
// void *pixels = malloc(*pixelSize);
// if (pixels == nullptr)
// 	throw RuntimeException(fvformatf("Failed to allocate %d, %s.\n", size, strerror(errno)));

// memcpy(pixels, pixel, *pixelSize);

// /*	Release free image resources.	*/
// FreeImage_Unload(firsbitmap);
// FreeImage_CloseMemory(stream);
// return pixels;
// }