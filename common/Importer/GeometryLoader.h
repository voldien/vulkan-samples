#ifndef _VK_SAMPLES_GEOMTRY_IMPORTER_H_
#define _VK_SAMPLES_GEOMTRY_IMPORTER_H_ 1
#include <vulkan/vulkan.h>
#include<string>

class GeometryLoader {
  public:
	static void loadMesh(const std::string& filename, VkDevice device, VkDeviceMemory &memory, VkBuffer &vertex,
						 VkBuffer &indices);
};

#endif
