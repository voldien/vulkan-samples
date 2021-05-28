#pragma once

class ImageImport {
	public:
	  static void *loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
								   TextureDesc::Format *pformat, TextureDesc::Format *pinternalformat,
								   TextureDesc::Type *ptype, unsigned long *pixelSize);
};
