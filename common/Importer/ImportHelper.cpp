#include "ImportHelper.h"
#include "ModelImporter.h"
#include "VKDataStructure.h"
#include "VKSampleBase.h"
#include "vulkan/vulkan_core.h"
#include <ImageUtil.h>
#include <ProceduralGeometry.h>
#include <cstdint>
#include <half.hpp>

using namespace vksample;
using namespace fragcore;

using ModelTemp = struct model_temp_t {
	const ModelSystemObject *model;
	size_t index;
};

// set_debug_marker_name

void ImportHelper::loadModelBuffer(VKSampleSessionBase &engine, ModelImporter &modelLoader,
								   std::vector<MeshObject> &modelSet) {

	modelSet.resize(modelLoader.getModels().size());
	unsigned int tmp_ibo = 0;

	MeshObject tmpMesh;

	std::map<int, std::vector<ModelTemp>> map;
	std::map<int, int> strideVBOMap;
	std::map<int, int> strideVBAMap;
	/*	*/
	size_t indices_offset = 0;
	size_t indicesDataSize = 0;

	/*	Sort based on vertex stride.	*/
	for (size_t i = 0; i < modelLoader.getModels().size(); i++) {
		const ModelSystemObject &refModel = modelLoader.getModels()[i];

		assert(refModel.vertexStride > 0);

		map[refModel.vertexStride].push_back({&refModel, i});
		indicesDataSize += refModel.indicesStride * refModel.nrIndices;
	}

	{
		/*	*/

		const VkMemoryPropertyFlags memFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		const VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		engine.allocateBuffer(indicesDataSize, useFlags, memFlag, tmpMesh.indicesBuffer, tmpMesh.indicesMemory);

		uint8_t *elementPointer = nullptr;
		//(uint8_t *)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, indicesDataSize, GL_MAP_WRITE_BIT);

		size_t offset = 0;

		for (auto it = map.begin(); it != map.end(); it++) {
			const std::vector<ModelTemp> &ref = (*it).second;
			const int vertexStride = (*it).first;

			for (size_t i = 0; i < ref.size(); i++) {
				const ModelSystemObject &refModel = *ref[i].model;
				const size_t indicesByteSize = refModel.indicesStride * refModel.nrIndices;

				engine.transferBufferData(tmpMesh.indicesBuffer, tmpMesh.indicesMemory,
										  (const void *)&elementPointer[offset], indicesByteSize, offset);
				//			std::memcpy(&elementPointer[offset], refModel.indicesData, indicesByteSize);
				offset += indicesByteSize;
			}
		}
	}

	/*	Create array buffer, for rendering static geometry.	*/
	size_t nrVertices = 0;
	size_t nrIndices = 0;
	size_t boneDataSize = 0;

	for (auto it = map.begin(); it != map.end(); it++) {

		const std::vector<ModelTemp> &ref = (*it).second;
		const int vertexStride = (*it).first;

		size_t vertexDataSize = 0;
		for (size_t ref_index = 0; ref_index < ref.size(); ref_index++) {
			const ModelSystemObject &refModel = *ref[ref_index].model;

			nrVertices += refModel.nrVertices;
			/*	*/
			vertexDataSize += refModel.vertexStride * refModel.nrVertices;
		}

		unsigned int tmp_vbo = 0;

		/*	Allocate memory.	*/
		const VkMemoryPropertyFlags memFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		const VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		engine.allocateBuffer(indicesDataSize, useFlags, memFlag, tmpMesh.vertexBuffer, tmpMesh.vertexMemory);

		uint8_t *vertexPointer =
			nullptr; // (uint8_t *)glMapBufferRange(GL_ARRAY_BUFFER, 0, vertexDataSize, GL_MAP_WRITE_BIT);

		/*	Map Buffer. */

		size_t offset = 0;

		for (size_t i = 0; i < ref.size(); i++) {
			const ModelSystemObject &refModel = *ref[i].model;

			const size_t vertexByteSize = refModel.vertexStride * refModel.nrVertices;

			engine.transferBufferData(tmpMesh.vertexBuffer, tmpMesh.vertexMemory, (const void *)&vertexPointer[offset],
									  vertexByteSize, offset);
			// std::memcpy(&vertexPointer[offset], refModel.vertexData, vertexByteSize);

			offset += vertexByteSize;
		}

		const ModelSystemObject &refModel_base = *ref[0].model;

		unsigned int tmp_vao = 0;

		/*	*/
		size_t vertices_offset = 0;
		for (size_t i = 0; i < ref.size(); i++) {

			/*	*/
			const size_t pindex = ref[i].index;
			const ModelSystemObject &refModel = *ref[i].model;

			/*	*/
			const size_t vertexStride = refModel.vertexStride;
			const size_t IndicesStride = refModel.indicesStride;

			/*	*/
			const size_t vertexSize = refModel.nrVertices;
			const size_t indicesSize = refModel.nrIndices;

			/*	*/
			MeshObject &ref = modelSet[pindex];
			ref.indices_offset = indices_offset;
			ref.vertex_offset = vertices_offset;
			ref.nrIndicesElements = refModel.nrIndices;
			ref.nrVertices = refModel.nrVertices;
			// ref.bound = refModel.bound;

			/*	*/
			vertices_offset += vertexSize;
			indices_offset += indicesSize;

			/*	*/
			ref.vertexBuffer = tmpMesh.vertexBuffer;
			ref.vertexMemory = tmpMesh.vertexMemory;
			ref.indicesBuffer = tmpMesh.indicesBuffer;
			ref.indicesMemory = tmpMesh.indicesMemory;

			switch (refModel.primitiveType) {
			case 1:
				ref.primitiveType = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				;
				break;
			case 2:
				ref.primitiveType = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				break;
			default:
			case 4:
				ref.primitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				break;
			}
		}
	}
}

// void ImportHelper::loadTextures(LainEngine &engine, ModelImporter &modelLoader,
// 								std::vector<TextureAssetObject> &textures) {

// 	std::vector<TextureAssetObject> &Reftextures = modelLoader.getTextures();

// 	std::vector<Image> images;
// 	images.resize(Reftextures.size());

// 	textures.resize(Reftextures.size());

// 	ImageImporter textureImporter(modelLoader.getFileSystem(), engine.getResourcePool());
// 	// MiscProcessingUtil process(modelLoader.getFileSystem());

// 	/*	*/
// #pragma omp parallel for schedule(dynamic, 4)
// 	for (size_t texture_index = 0; texture_index < Reftextures.size(); texture_index++) {

// 		std::vector<MaterialObject *> materials = modelLoader.getMaterials(texture_index);

// 		TextureAssetObject &tex = Reftextures[texture_index];
// 		TextureCompression compression = TextureCompression::Default;

// 		if (tex.data == nullptr) {

// 			try {
// 				std::cout << "Loading " << tex.filepath << std::endl;
// 				fragcore::Ref<fragcore::IO> refIO =
// 					fragcore::Ref<fragcore::IO>(new fragcore::FileIO(tex.filepath, FileIO::READ));

// 				/*	*/
// 				fragcore::ImageLoader imageLoader;
// 				Image image = imageLoader.loadImage(refIO);

// 				/*	Convert BumpMap to NormalMap*/
// 				// if (!materials.empty() && materials[0]->heightbumpIndex == texture_index) {
// 				//	// TODO: use gpu to convert image.
// 				//	image = std::move(ImageUtil::convert2NormalMap(image, 3.5f));
// 				//	materials[0]->heightbumpIndex = -1;
// 				//	materials[0]->normalIndex = texture_index;
// 				// }

// 				/*	*/
// 				images[texture_index] = std::move(image);
// 				// tex.texture = textureImporter.loadImage2DRaw(image, colorSpace, compression);

// 			} catch (const std::exception &ex) {
// 				std::cerr << "Failed to load: " << tex.filepath << " " << ex.what() << std::endl;
// 			}
// 		} else {

// 			/*	Compressed data.	*/
// 			if (tex.height == 0 && tex.dataSize > 0 && tex.data && tex.width > 0) {

// 				/*	*/
// 				fragcore::Ref<fragcore::IO> refIO = fragcore::Ref<fragcore::IO>(
// 					new fragcore::BufferIO((const void *)tex.data, (unsigned long)tex.dataSize));

// 				/*	*/
// 				fragcore::ImageLoader imageLoader;

// 				try {

// 					Image image = imageLoader.loadImage(refIO);
// 					images[texture_index] = std::move(image);
// 					// tex.texture = textureImporter.loadImage2DRaw(image, colorSpace, compression);

// 				} catch (std::exception &ex) {
// 					std::cerr << "Failed to load: " << ex.what() << std::endl;
// 				}

// 				refIO->close();
// 			} else {

// 				/*	None-compressed.	*/
// 				fragcore::Image image(tex.width, tex.height, ImageFormat::ARGB32);
// 				image.setPixelData(tex.data, image.getSize());
// 				images[texture_index] = std::move(image);
// 				/*	*/
// 				// tex.texture = textureImporter.loadImage2DRaw(image, colorSpace, compression);
// 			}
// 		}

// 		/*	*/
// 	}

// 	std::cout << "Start Transfering ImageData to GPU" << std::endl;
// #pragma omp master
// 	for (size_t texture_index = 0; texture_index < images.size(); texture_index++) {

// 		TextureAssetObject &tex = Reftextures[texture_index];
// 		ColorSpace colorSpace = ColorSpace::RawLinear;
// 		const TextureCompression compression = TextureCompression::Default;

// 		std::vector<MaterialObject *> materials = modelLoader.getMaterials(texture_index);

// 		/*	Determine color space, based on the texture usages.	*/
// 		// if (!materials.empty()) {
// 		//	if (materials[0]->diffuseIndex == texture_index) {
// 		//		colorSpace = ColorSpace::SRGB;
// 		//	}
// 		// }

// 		/*	*/
// 		//	tex.texture = textureImporter.loadImage2DRaw(images[texture_index], colorSpace, compression);

// 		/*	*/
// 		// if (tex.texture >= 0) {
// 		//	glObjectLabel(GL_TEXTURE, tex.texture, tex.filepath.size(), tex.filepath.data());
// 		// }
// 	}

// 	textures = Reftextures;
// }