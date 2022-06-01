#include "Importer/ImageImport.h"
#include "VKUtil.h"
#include "VksCommon.h"
#include <cxxopts.hpp>
#include <fmt/format.h>

class ShaderInfo : public VKSampleSession {
  public:
	ShaderInfo(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device) : VKSampleSession(core, device) {}

	virtual ~ShaderInfo() {}

	VkPipeline loadPipeline() { return VK_NULL_HANDLE; }

	virtual void run() override {

		std::vector<VkPipeline> pipelines = {};

		PFN_vkGetShaderInfoAMD pfnGetShaderInfoAMD =
			(PFN_vkGetShaderInfoAMD)vkGetDeviceProcAddr(this->getDevice(), "vkGetShaderInfoAMD");

		for (size_t i = 0; i < pipelines.size(); i++) {
			VkPipeline gfxPipeline = pipelines[i];

			VkShaderStatisticsInfoAMD statistics = {};
			size_t dataSize = sizeof(statistics);

			if (pfnGetShaderInfoAMD(this->getDevice(), gfxPipeline, VK_SHADER_STAGE_FRAGMENT_BIT,
									VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD, &dataSize, nullptr) == VK_SUCCESS) {
				printf("Fragment shader disassembly:\n");

				void *disassembly = malloc(dataSize);

				if (pfnGetShaderInfoAMD(this->getDevice(), gfxPipeline, VK_SHADER_STAGE_FRAGMENT_BIT,
										VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD, &dataSize, disassembly) == VK_SUCCESS) {

					printf((char *)disassembly);
				}

				free(disassembly);
			}
		}
	}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {{VK_AMD_SHADER_INFO_EXTENSION_NAME, true}};

	try {
		VKSampleWindow<ShaderInfo> sample(argc, argv, required_device_extensions, {}, required_instance_extensions);
		sample.run();

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}