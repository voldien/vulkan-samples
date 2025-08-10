#include "MeshProcedural.h"
#include "VKDataStructure.h"
#include "VKSampleBase.h"
#include "vulkan/vulkan_core.h"
#include <ProceduralGeometry.h>
#include <internal_object_type.h>

using namespace vksample;
using namespace fragcore;

MeshProcedural::MeshProcedural(VKSampleSessionBase &vkBase) : vkBase(vkBase) {}

void MeshProcedural::loadPlan(MeshObject &planMesh, const float scale, const int segmentX, const int segmentY) {
	std::vector<ProceduralGeometry::Vertex> vertices;
	vertices.reserve(sizeof(ProceduralGeometry::Vertex) * 1024);
	std::vector<unsigned int> indices;
	indices.reserve(sizeof(unsigned int) * 1024);
	ProceduralGeometry::generatePlan(scale, vertices, indices, segmentX, segmentY);

	const unsigned int stride = sizeof(ProceduralGeometry::Vertex);

	const size_t vertices_size = vertices.size() * stride;
	const size_t indicies_size = indices.size() * sizeof(indices[0]);
	const size_t total_memory_size = indicies_size + vertices_size;

	const VkMemoryPropertyFlags memFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	const VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	vkBase.allocateBuffer(vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | useFlags, memFlag,
						  planMesh.vertexBuffer, planMesh.vertexMemory);
	vkBase.allocateBuffer(indicies_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | useFlags, memFlag,
						  planMesh.indicesBuffer, planMesh.indicesMemory);

	/*	*/
	vkBase.transferBufferData(planMesh.vertexBuffer, planMesh.vertexMemory, vertices.data(), vertices_size);
	vkBase.transferBufferData(planMesh.indicesBuffer, planMesh.indicesMemory, indices.data(), indicies_size);

	planMesh.nrIndicesElements = indices.size();
	planMesh.indices_offset = 0;
	planMesh.vertex_offset = 0;
	planMesh.nrVertices = vertices.size();
	planMesh.stride = stride;
}

void MeshProcedural::loadCube(MeshObject &cubeMesh, const float scale, const int segmentX, const int segmentY) {

	std::vector<ProceduralGeometry::Vertex> vertices;
	vertices.reserve(sizeof(ProceduralGeometry::Vertex) * 1024);
	std::vector<unsigned int> indices;
	indices.reserve(sizeof(unsigned int) * 1024);
	ProceduralGeometry::generateCube(scale, vertices, indices, segmentX);
	const unsigned int stride = sizeof(ProceduralGeometry::Vertex);

	const size_t vertices_size = vertices.size() * stride;
	const size_t indicies_size = indices.size() * sizeof(indices[0]);
	const size_t total_memory_size = indicies_size + vertices_size;

	const VkMemoryPropertyFlags memFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	const VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	vkBase.allocateBuffer(vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | useFlags, memFlag,
						  cubeMesh.vertexBuffer, cubeMesh.vertexMemory);
	vkBase.allocateBuffer(indicies_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | useFlags, memFlag,
						  cubeMesh.indicesBuffer, cubeMesh.indicesMemory);

	/*	*/
	vkBase.transferBufferData(cubeMesh.vertexBuffer, cubeMesh.vertexMemory, vertices.data(), vertices_size);
	vkBase.transferBufferData(cubeMesh.indicesBuffer, cubeMesh.indicesMemory, indices.data(), indicies_size);

	/*	*/
	cubeMesh.nrIndicesElements = indices.size();
	cubeMesh.indices_offset = 0;
	cubeMesh.vertex_offset = 0;
	cubeMesh.nrVertices = vertices.size();
	cubeMesh.stride = stride;
}

void MeshProcedural::loadSphere(MeshObject &sphereMesh, const float radius, const int slices, const int segements) {
	std::vector<ProceduralGeometry::Vertex> vertices;
	vertices.reserve(sizeof(ProceduralGeometry::Vertex) * 1024);
	std::vector<unsigned int> indices;
	indices.reserve(sizeof(unsigned int) * 1024);
	ProceduralGeometry::generateSphere(radius, vertices, indices, slices, segements);

	const unsigned int stride = sizeof(ProceduralGeometry::Vertex);

	const size_t vertices_size = vertices.size() * stride;
	const size_t indicies_size = indices.size() * sizeof(indices[0]);
	const size_t total_memory_size = indicies_size + vertices_size;

	const VkMemoryPropertyFlags memFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	const VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	vkBase.allocateBuffer(vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | useFlags, memFlag,
						  sphereMesh.vertexBuffer, sphereMesh.vertexMemory);
	vkBase.allocateBuffer(indicies_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | useFlags, memFlag,
						  sphereMesh.indicesBuffer, sphereMesh.indicesMemory);

	/*	*/
	vkBase.transferBufferData(sphereMesh.vertexBuffer, sphereMesh.vertexMemory, vertices.data(), vertices_size, 0);
	vkBase.transferBufferData(sphereMesh.indicesBuffer, sphereMesh.indicesMemory, indices.data(), indicies_size, 0);

	sphereMesh.nrIndicesElements = indices.size();
	sphereMesh.indices_offset = 0;
	sphereMesh.vertex_offset = 0;
	sphereMesh.nrVertices = vertices.size();
	sphereMesh.stride = stride;
}
