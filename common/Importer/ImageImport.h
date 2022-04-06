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
	/**
	 * @brief
	 *
	 */
	enum Format { // GraphicsFormat
		NoFormat,
		RGB = 0x1,	/*	RGB components.	*/
		RGBA = 0x2, /*	RGBA components.	*/
		BGR = 0x3,	/*	BGR components.	*/
		BGRA = 0x4, /*	BGRA components.	*/
		SRGB = 0x5, /*	SRGB components.	*/
		RG = 0x6,
		A = 0x8,
		SRGBA = 0x7,		/*	SRGBA components.	*/
		SingleColor = 0x9,	/*	Single color component.	*/
		Depth = 0xA,		/*	Depth component.	*/
		Stencil = 0xB,		/*	Stencil component.	*/
		DepthStencil = 0xC, /*	Depth and stencil componets.	*/
	};

	enum GraphicFormat {
		NoneGraphicFormat, // The fvformatf is not specified.
		R8_SRGB, // A one-component, 8-bit unsigned normalized fvformatf that has a single 8-bit R component stored with
				 // sRGB nonlinear encoding.
		R8G8_SRGB, // A two-component, 16-bit unsigned normalized fvformatf that has an 8-bit R component stored with
				   // sRGB nonlinear encoding in byte 0, and an 8-bit G component stored with sRGB nonlinear encoding in
				   // byte 1.
		R8G8B8_SRGB, // A three-component, 24-bit unsigned normalized fvformatf that has an 8-bit R component stored
					 // with sRGB nonlinear encoding in byte 0, an 8-bit G component stored with sRGB nonlinear encoding
					 // in byte 1, and an 8-bit B component stored with sRGB nonlinear encoding in byte 2.
		R8G8B8A8_SRGB, // A four-component, 32-bit unsigned normalized fvformatf that has an 8-bit R component stored
					   // with sRGB nonlinear encoding in byte 0, an 8-bit G component stored with sRGB nonlinear
					   // encoding in byte 1, an 8-bit B component stored with sRGB nonlinear encoding in byte 2, and an
					   // 8-bit A component in byte 3.
		R8_UNorm,	   // A one-component, 8-bit unsigned normalized fvformatf that has a single 8-bit R component.
		R8G8_UNorm,	  // A two-component, 16-bit unsigned normalized fvformatf that has an 8-bit R component stored with
					  // sRGB nonlinear encoding in byte 0, and an 8-bit G component stored with sRGB nonlinear encoding
					  // in byte 1.
		R8G8B8_UNorm, // A three-component, 24-bit unsigned normalized fvformatf that has an 8-bit R component in byte
					  // 0, an 8-bit G component in byte 1, and an 8-bit B component in byte 2.
		R8G8B8A8_UNorm, //	A four-component, 32-bit unsigned normalized fvformatf that has an 8-bit R component in byte
						// 0, an 8-bit G component in byte 1, an 8-bit B component in byte 2, and an 8-bit A component
						// in byte 3.
		R8_SNorm,	  //	A one-component, 8-bit signed normalized fvformatf that has a single 8-bit R component.
		R8G8_SNorm,	  //	A two-component, 16-bit signed normalized fvformatf that has an 8-bit R component stored with
					  // sRGB nonlinear encoding in byte 0, and an 8-bit G component stored with sRGB nonlinear encoding
					  // in byte 1.
		R8G8B8_SNorm, //	A three-component, 24-bit signed normalized fvformatf that has an 8-bit R component in byte
					  // 0, an 8-bit G component in byte 1, and an 8-bit B component in byte 2.
		R8G8B8A8_SNorm, //	A four-component, 32-bit signed normalized fvformatf that has an 8-bit R component in byte
						// 0, an 8-bit G component in byte 1, an 8-bit B component in byte 2, and an 8-bit A component
						// in byte 3.
		R8_UInt,   //	A one-component, 8-bit unsigned integer fvformatf that has a single 8-bit R component.
		R8G8_UInt, //	A two-component, 16-bit unsigned integer fvformatf that has an 8-bit R component in byte 0, and
				   // an 8-bit G component in byte 1.
		R8G8B8_UInt, //	A three-component, 24-bit unsigned integer fvformatf that has an 8-bit R component in byte 0, an
					 // 8-bit G component in byte 1, and an 8-bit B component in byte 2.
		R8G8B8A8_UInt, //	A four-component, 32-bit unsigned integer fvformatf that has an 8-bit R component in byte 0,
					   // an 8-bit G component in byte 1, an 8-bit B component in byte 2, and an 8-bit A component in
					   // byte 3.
		R8_SInt,	   //	A one-component, 8-bit signed integer fvformatf that has a single 8-bit R component.
		R8G8_SInt, //	A two-component, 16-bit signed integer fvformatf that has an 8-bit R component in byte 0, and an
				   // 8-bit G component in byte 1.
		R8G8B8_SInt, //	A three-component, 24-bit signed integer fvformatf that has an 8-bit R component in byte 0, an
					 // 8-bit G component in byte 1, and an 8-bit B component in byte 2.
		R8G8B8A8_SInt,	 //	A four-component, 32-bit signed integer fvformatf that has an 8-bit R component in byte 0,
						 // an 8-bit G component in byte 1, an 8-bit B component in byte 2, and an 8-bit A component in
						 // byte 3.
		R16_UNorm,		 //	A one-component, 16-bit unsigned normalized fvformatf that has a single 16-bit R component.
		R16G16_UNorm,	 //	A two-component, 32-bit unsigned normalized fvformatf that has a 16-bit R component in bytes
						 // 0..1, and a 16-bit G component in bytes 2..3.
		R16G16B16_UNorm, //	A three-component, 48-bit unsigned normalized fvformatf that has a 16-bit R component in
						 // bytes 0..1, a 16-bit G component in bytes 2..3, and a 16-bit B component in bytes 4..5.
		R16G16B16A16_UNorm, //	A four-component, 64-bit unsigned normalized fvformatf that has a 16-bit R component in
							// bytes 0..1, a 16-bit G component in bytes 2..3, a 16-bit B component in bytes 4..5, and a
							// 16-bit A component in bytes 6..7.
		R16_SNorm,			//	A one-component, 16-bit signed normalized fvformatf that has a single 16-bit R component.
		R16G16_SNorm,	 //	A two-component, 32-bit signed normalized fvformatf that has a 16-bit R component in bytes
						 // 0..1, and a 16-bit G component in bytes 2..3.
		R16G16B16_SNorm, //	A three-component, 48-bit signed normalized fvformatf that has a 16-bit R component in bytes
						 // 0..1, a 16-bit G component in bytes 2..3, and a 16-bit B component in bytes 4..5.
		R16G16B16A16_SNorm, //	A four-component, 64-bit signed normalized fvformatf that has a 16-bit R component in
							// bytes 0..1, a 16-bit G component in bytes 2..3, a 16-bit B component in bytes 4..5, and a
							// 16-bit A component in bytes 6..7.
		R16_UInt,			//	A one-component, 16-bit unsigned integer fvformatf that has a single 16-bit R component.
		R16G16_UInt, //	A two-component, 32-bit unsigned integer fvformatf that has a 16-bit R component in bytes 0..1,
					 // and a 16-bit G component in bytes 2..3.
		R16G16B16_UInt, //	A three-component, 48-bit unsigned integer fvformatf that has a 16-bit R component in bytes
						// 0..1, a 16-bit G component in bytes 2..3, and a 16-bit B component in bytes 4..5.
		R16G16B16A16_UInt, //	A four-component, 64-bit unsigned integer fvformatf that has a 16-bit R component in
						   // bytes 0..1, a 16-bit G component in bytes 2..3, a 16-bit B component in bytes 4..5, and a
						   // 16-bit A component in bytes 6..7.
		R16_SInt,		   //	A one-component, 16-bit signed integer fvformatf that has a single 16-bit R component.
		R16G16_SInt, //	A two-component, 32-bit signed integer fvformatf that has a 16-bit R component in bytes 0..1,
					 // and a 16-bit G component in bytes 2..3.
		R16G16B16_SInt, //	A three-component, 48-bit signed integer fvformatf that has a 16-bit R component in bytes
						// 0..1, a 16-bit G component in bytes 2..3, and a 16-bit B component in bytes 4..5.
		R16G16B16A16_SInt, //	A four-component, 64-bit signed integer fvformatf that has a 16-bit R component in bytes
						   // 0..1, a 16-bit G component in bytes 2..3, a 16-bit B component in bytes 4..5, and a 16-bit
						   // A component in bytes 6..7.
		R32_UInt,		   //	A one-component, 32-bit unsigned integer fvformatf that has a single 32-bit R component.
		R32G32_UInt, //	A two-component, 64-bit unsigned integer fvformatf that has a 32-bit R component in bytes 0..3,
					 // and a 32-bit G component in bytes 4..7.
		R32G32B32_UInt, //	A three-component, 96-bit unsigned integer fvformatf that has a 32-bit R component in bytes
						// 0..3, a 32-bit G component in bytes 4..7, and a 32-bit B component in bytes 8..11.
		R32G32B32A32_UInt, //	A four-component, 128-bit unsigned integer fvformatf that has a 32-bit R component in
						   // bytes 0..3, a 32-bit G component in bytes 4..7, a 32-bit B component in bytes 8..11, and a
						   // 32-bit A component in bytes 12..15.
		R32_SInt,		   //	A one-component, 32-bit signed integer fvformatf that has a single 32-bit R component.
		R32G32_SInt, //	A two-component, 64-bit signed integer fvformatf that has a 32-bit R component in bytes 0..3,
					 // and a 32-bit G component in bytes 4..7.
		R32G32B32_SInt, //	A three-component, 96-bit signed integer fvformatf that has a 32-bit R component in bytes
						// 0..3, a 32-bit G component in bytes 4..7, and a 32-bit B component in bytes 8..11.
		R32G32B32A32_SInt, //	A four-component, 128-bit signed integer fvformatf that has a 32-bit R component in
						   // bytes 0..3, a 32-bit G component in bytes 4..7, a 32-bit B component in bytes 8..11, and a
						   // 32-bit A component in bytes 12..15.
		R16_SFloat,	   //	A one-component, 16-bit signed floating-point fvformatf that has a single 16-bit R component.
		R16G16_SFloat, //	A two-component, 32-bit signed floating-point fvformatf that has a 16-bit R component in
					   // bytes 0..1, and a 16-bit G component in bytes 2..3.
		R16G16B16_SFloat, //	A three-component, 48-bit signed floating-point fvformatf that has a 16-bit R component
						  // in bytes 0..1, a 16-bit G component in bytes 2..3, and a 16-bit B component in bytes 4..5.
		R16G16B16A16_SFloat, //	A four-component, 64-bit signed floating-point fvformatf that has a 16-bit R component
							 // in bytes 0..1, a 16-bit G component in bytes 2..3, a 16-bit B component in bytes 4..5,
							 // and a 16-bit A component in bytes 6..7.
		R32_SFloat,	   //	A one-component, 32-bit signed floating-point fvformatf that has a single 32-bit R component.
		R32G32_SFloat, //	A two-component, 64-bit signed floating-point fvformatf that has a 32-bit R component in
					   // bytes 0..3, and a 32-bit G component in bytes 4..7.
		R32G32B32_SFloat, //	A three-component, 96-bit signed floating-point fvformatf that has a 32-bit R component
						  // in bytes 0..3, a 32-bit G component in bytes 4..7, and a 32-bit B component in bytes 8..11.
		R32G32B32A32_SFloat, //	A four-component, 128-bit signed floating-point fvformatf that has a 32-bit R component
							 // in bytes 0..3, a 32-bit G component in bytes 4..7, a 32-bit B component in bytes 8..11,
							 // and a 32-bit A component in bytes 12..15.
		B8G8R8_SRGB, //	A three-component, 24-bit unsigned normalized fvformatf that has an 8-bit R component stored
					 // with sRGB nonlinear encoding in byte 0, an 8-bit G component stored with sRGB nonlinear encoding
					 // in byte 1, and an 8-bit B component stored with sRGB nonlinear encoding in byte 2.
		B8G8R8A8_SRGB, //	A four-component, 32-bit unsigned normalized fvformatf that has an 8-bit B component stored
					   // with sRGB nonlinear encoding in byte 0, an 8-bit G component stored with sRGB nonlinear
					   // encoding in byte 1, an 8-bit R component stored with sRGB nonlinear encoding in byte 2, and an
					   // 8-bit A component in byte 3.
		B8G8R8_UNorm,  //	A three-component, 24-bit unsigned normalized fvformatf that has an 8-bit B component in
					   // byte 0, an 8-bit G component in byte 1, and an 8-bit R component in byte 2.
		B8G8R8A8_UNorm, //	A four-component, 32-bit unsigned normalized fvformatf that has an 8-bit B component in byte
						// 0, an 8-bit G component in byte 1, an 8-bit R component in byte 2, and an 8-bit A component
						// in byte 3.
		B8G8R8_SNorm, //	A three-component, 24-bit signed normalized fvformatf that has an 8-bit B component in byte
					  // 0, an 8-bit G component in byte 1, and an 8-bit R component in byte 2.
		B8G8R8A8_SNorm, //	A four-component, 32-bit signed normalized fvformatf that has an 8-bit B component in byte
						// 0, an 8-bit G component in byte 1, an 8-bit R component in byte 2, and an 8-bit A component
						// in byte 3.
		B8G8R8_UInt, //	A three-component, 24-bit unsigned integer fvformatf that has an 8-bit B component in byte 0, an
					 // 8-bit G component in byte 1, and an 8-bit R component in byte 2
		B8G8R8A8_UInt, //	A four-component, 32-bit unsigned integer fvformatf that has an 8-bit B component in byte 0,
					   // an 8-bit G component in byte 1, an 8-bit R component in byte 2, and an 8-bit A component in
					   // byte 3.
		B8G8R8_SInt,   //	A three-component, 24-bit signed integer fvformatf that has an 8-bit B component in byte 0, an
					   // 8-bit G component in byte 1, and an 8-bit R component in byte 2.
		B8G8R8A8_SInt, //	A four-component, 32-bit signed integer fvformatf that has an 8-bit B component in byte 0,
					   // an 8-bit G component in byte 1, an 8-bit R component in byte 2, and an 8-bit A component in
					   // byte 3.
		R4G4B4A4_UNormPack16, //	A four-component, 16-bit packed unsigned normalized fvformatf that has a 4-bit R
							  // component in bits 12..15, a 4-bit G component in bits 8..11, a 4-bit B component in
							  // bits 4..7, and a 4-bit A component in bits 0..3.
		B4G4R4A4_UNormPack16, //	A four-component, 16-bit packed unsigned normalized fvformatf that has a 4-bit B
							  // component in bits 12..15, a 4-bit G component in bits 8..11, a 4-bit R component in
							  // bits 4..7, and a 4-bit A component in bits 0..3.
		R5G6B5_UNormPack16,	  //	A three-component, 16-bit packed unsigned normalized fvformatf that has a 5-bit R
							  // component in bits 11..15, a 6-bit G component in bits 5..10, and a 5-bit B component in
							  // bits 0..4.
		B5G6R5_UNormPack16,	  //	A three-component, 16-bit packed unsigned normalized fvformatf that has a 5-bit B
							  // component in bits 11..15, a 6-bit G component in bits 5..10, and a 5-bit R component in
							  // bits 0..4.
		R5G5B5A1_UNormPack16, //	A four-component, 16-bit packed unsigned normalized fvformatf that has a 5-bit R
							  // component in bits 11..15, a 5-bit G component in bits 6..10, a 5-bit B component in
							  // bits 1..5, and a 1-bit A component in bit 0.
		B5G5R5A1_UNormPack16, //	A four-component, 16-bit packed unsigned normalized fvformatf that has a 5-bit B
							  // component in bits 11..15, a 5-bit G component in bits 6..10, a 5-bit R component in
							  // bits 1..5, and a 1-bit A component in bit 0.
		A1R5G5B5_UNormPack16, //	A four-component, 16-bit packed unsigned normalized fvformatf that has a 1-bit A
							  // component in bit 15, a 5-bit R component in bits 10..14, a 5-bit G component in
							  // bits 5..9, and a 5-bit B component in bits 0..4.
		E5B9G9R9_UFloatPack32, //	A three-component, 32-bit packed unsigned floating-point fvformatf that has a 5-bit
							   // shared exponent in bits 27..31, a 9-bit B component mantissa in bits 18..26, a 9-bit G
							   // component mantissa in bits 9..17, and a 9-bit R component mantissa in bits 0..8.
		B10G11R11_UFloatPack32,	 //	A three-component, 32-bit packed unsigned floating-point fvformatf that has a 10-bit
								 // B component in bits 22..31, an 11-bit G component in bits 11..21, an 11-bit R
								 // component in bits 0..10.
		A2B10G10R10_UNormPack32, //	A four-component, 32-bit packed unsigned normalized fvformatf that has a 2-bit A
								 // component in bits 30..31, a 10-bit B component in bits 20..29, a 10-bit G component
								 // in bits 10..19, and a 10-bit R component in bits 0..9.
		A2B10G10R10_UIntPack32,	 //	A four-component, 32-bit packed unsigned integer fvformatf that has a 2-bit A
								 // component in bits 30..31, a 10-bit B component in bits 20..29, a 10-bit G component
								 // in bits 10..19, and a 10-bit R component in bits 0..9.
		A2B10G10R10_SIntPack32,	 //	A four-component, 32-bit packed signed integer fvformatf that has a 2-bit A
								 // component in bits 30..31, a 10-bit B component in bits 20..29, a 10-bit G component
								 // in bits 10..19, and a 10-bit R component in bits 0..9.
		A2R10G10B10_UNormPack32, //	A four-component, 32-bit packed unsigned normalized fvformatf that has a 2-bit A
								 // component in bits 30..31, a 10-bit R component in bits 20..29, a 10-bit G component
								 // in bits 10..19, and a 10-bit B component in bits 0..9.
		A2R10G10B10_UIntPack32,	 //	A four-component, 32-bit packed unsigned integer fvformatf that has a 2-bit A
								 // component in bits 30..31, a 10-bit R component in bits 20..29, a 10-bit G component
								 // in bits 10..19, and a 10-bit B component in bits 0..9.
		A2R10G10B10_SIntPack32,	 //	A four-component, 32-bit packed signed integer fvformatf that has a 2-bit A
	};

	/**
	 * @brief
	 *
	 */
	enum TextureFormat {
		Alpha8,	  // Alpha-only texture fvformatf.
		ARGB4444, //	A 16 bits/pixel texture fvformatf. Texture stores color with an alpha channel.
		RGB24,	  //	Color texture fvformatf, 8-bits per channel.
		RGBA32,	  //	Color with alpha texture fvformatf, 8-bits per channel.
		ARGB32,	  //	Color with alpha texture fvformatf, 8-bits per channel.
		RGB565,	  //	A 16 bit color texture fvformatf.
		R16,	  //	Single channel (R) texture fvformatf, 16 bit integer.
		DXT1,	  //	Compressed color texture fvformatf.
		DXT5,	  //	Compressed color with alpha channel texture fvformatf.
		RGBA4444, //	Color and alpha texture fvformatf, 4 bit per channel.
		BGRA32,	  //	Color with alpha texture fvformatf, 8-bits per channel.
		BGR24,
		RHalf,		   //	Scalar (R) texture fvformatf, 16 bit floating point.
		RGHalf,		   //	Two color (RG) texture fvformatf, 16 bit floating point per channel.
		RGBAHalf,	   //	RGB color and alpha texture fvformatf, 16 bit floating point per channel.
		RFloat,		   //	Scalar (R) texture fvformatf, 32 bit floating point.
		RGFloat,	   //	Two color (RG) texture fvformatf, 32 bit floating point per channel.
		RGBAFloat,	   //	RGB color and alpha texture fvformatf, 32-bit floats per channel.
		YUY2,		   //	A format that uses the YUV color space and is often used for video encoding or playback.
		RGB9e5Float,   //	RGB HDR fvformatf, with 9 bit mantissa per channel and a 5 bit shared exponent.
		BC4,		   //	Compressed one channel (R) texture fvformatf.
		BC5,		   //	Compressed two-channel (RG) texture fvformatf.
		BC6H,		   //	HDR compressed color texture fvformatf.
		BC7,		   //	High quality compressed color texture fvformatf.
		DXT1Crunched,  //	Compressed color texture fvformatf with Crunch compression for smaller storage sizes.
		DXT5Crunched,  //	Compressed color with alpha channel texture fvformatf with Crunch compression for smaller
					   // storage sizes.
		PVRTC_RGB2,	   //	PowerVR (iOS) 2 bits/pixel compressed color texture fvformatf.
		PVRTC_RGBA2,   // width	PowerVR (iOS) 2 bits/pixel compressed with alpha channel texture fvformatf.
		PVRTC_RGB4,	   //	PowerVR (iOS) 4 bits/pixel compressed color texture fvformatf.
		PVRTC_RGBA4,   //	PowerVR (iOS) 4 bits/pixel compressed with alpha channel texture fvformatf.
		ETC_RGB4,	   //	ETC (GLES2.0) 4 bits/pixel compressed RGB texture fvformatf.
		EAC_R,		   // / EAC (GL ES 3.0) 4 bits/pixel compressed unsigned single-channel texture fvformatf.
		EAC_R_SIGNED,  //	ETC2 / EAC (GL ES 3.0) 4 bits/pixel compressed signed single-channel texture fvformatf.
		EAC_RG,		   //	ETC2 / EAC (GL ES 3.0) 8 bits/pixel compressed unsigned dual-channel (RG) texture fvformatf.
		EAC_RG_SIGNED, //	ETC2 / EAC (GL ES 3.0) 8 bits/pixel compressed signed dual-channel (RG) texture fvformatf.
		ETC2_RGB,	   //	ETC2 (GL ES 3.0) 4 bits/pixel compressed RGB texture fvformatf.
		ETC2_RGBA1,	   //	ETC2 (GL ES 3.0) 4 bits/pixel RGB+1-bit alpha texture fvformatf.
		ETC2_RGBA8,	   //	ETC2 (GL ES 3.0) 8 bits/pixel compressed RGBA texture fvformatf.
		ASTC_4x4,	   //	ASTC (4x4 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		ASTC_5x5,	   //	ASTC (5x5 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		ASTC_6x6,	   //	ASTC (6x6 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		ASTC_8x8,	   //	ASTC (8x8 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		ASTC_10x10,	   //	ASTC (10x10 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		ASTC_12x12,	   //	ASTC (12x12 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		RG16,		   //	Two color (RG) texture fvformatf, 8-bits per channel.
		R8,			   //	Single channel (R) texture fvformatf, 8 bit integer.
		ETC_RGB4Crunched,	//	Compressed color texture fvformatf with Crunch compression for smaller storage sizes.
		ETC2_RGBA8Crunched, //	Compressed color with alpha channel texture fvformatf using Crunch compression for
							// smaller storage sizes.
		ASTC_HDR_4x4,		//	ASTC (4x4 pixel block in 128 bits) compressed RGB(A) HDR texture fvformatf.
		ASTC_HDR_5x5,		//	ASTC (5x5 pixel block in 128 bits) compressed RGB(A) HDR texture fvformatf.
		ASTC_HDR_6x6,		//	ASTC (6x6 pixel block in 128 bits) compressed RGB(A) HDR texture fvformatf.
		ASTC_HDR_8x8,		//	ASTC (8x8 pixel block in 128 bits) compressed RGB(A) texture fvformatf.
		ASTC_HDR_10x10,		//	ASTC (10x10 pixel block in 128 bits) compressed RGB(A) HDR texture fvformatf.
		ASTC_HDR_12x12,		//	ASTC (12x12 pixel block in 128 bits) compressed RGB(A) HDR texture fvformatf.
		ASTC_RGB_4x4,		//	ASTC (4x4 pixel block in 128 bits) compressed RGB texture fvformatf.
		ASTC_RGB_5x5,		//	ASTC (5x5 pixel block in 128 bits) compressed RGB texture fvformatf.
		ASTC_RGB_6x6,		//	ASTC (6x6 pixel block in 128 bits) compressed RGB texture fvformatf.
		ASTC_RGB_8x8,		//	ASTC (8x8 pixel block in 128 bits) compressed RGB texture fvformatf.
		ASTC_RGB_10x10,		//	ASTC (10x10 pixel block in 128 bits) compressed RGB texture fvformatf.
		ASTC_RGB_12x12,		//	ASTC (12x12 pixel block in 128 bits) compressed RGB texture fvformatf.
		ASTC_RGBA_4x4,		//	ASTC (4x4 pixel block in 128 bits) compressed RGBA texture fvformatf.
		ASTC_RGBA_5x5,		//	ASTC (5x5 pixel block in 128 bits) compressed RGBA texture fvformatf.
		ASTC_RGBA_6x6,		//	ASTC (6x6 pixel block in 128 bits) compressed RGBA texture fvformatf.
		ASTC_RGBA_8x8,		//	ASTC (8x8 pixel block in 128 bits) compressed RGBA texture fvformatf.
		ASTC_RGBA_10x10,	//	ASTC (10x10 pixel block in 128 bits) compressed RGBA texture fvformatf.
		ASTC_RGBA_12x12,	//	ASTC (12x12 pixel block in 128 bits) compressed RGBA texture fvformatf.
	};

	/**
	 * @brief
	 *
	 */
	enum Type {
		NoType,
		UnsignedByte = 0x1, /*	Each color component encoded in a single byte.	*/
		SignedByte = 0x2,	/*	Each color component encoded in a single signed byte.	*/
		UnsignedShort = 0x3,
		SignedShort = 0x4,
		UnsignedInt = 0x5,
		SignedInt = 0x6,
		Float = 0x7,		/*	Each color component encoded in a single float.	*/
		HalfFloat = 0x8,	/*  */
		Double = 0x9,		/*  */
		Unsigned24_8 = 0xA, /*	Each color component encoded.	*/
	};

  public:
	static void *loadTextureData(const char *cfilename, unsigned int *pwidth, unsigned int *pheight,
								 unsigned int *pformat, unsigned int *pinternalformat, unsigned int *ptype,
								 unsigned long *pixelSize);
	static void saveTextureData(const char *cfilename, const void *pixelData, unsigned int width, unsigned int height,
								int layers, unsigned int format);

	static void saveTextureData(const char *filename, VkDevice device, VkImage image);

	static void createImage(const char *filename, const VKDevice &device, VkImage &image);

	static void createImage2D(const char *filename, VkDevice device, VkCommandPool commandPool, VkQueue queue,
							  VkPhysicalDevice physicalDevice, VkImage &textureImage,
							  VkDeviceMemory &textureImageMemory);

	static void createCubeMap(const char *Xplus, VkDevice device, VkCommandPool commandPool, VkQueue queue,
							  VkPhysicalDevice physicalDevice, VkImage &textureImage,
							  VkDeviceMemory &textureImageMemory);
};

#endif
