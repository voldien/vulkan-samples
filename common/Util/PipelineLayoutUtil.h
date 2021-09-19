#pragma once

#include<spirv-reflect/spirv_reflect.h>
#include<vulkan.hpp>

using namespace spv_reflect;

class PipelineLayoutUtil {
  public:

	void layout(&std::vector<char>& module){
		SpvReflectShaderModule spvModule;
		spvReflectCreateShaderModule(module.size(), module.data(), &spvModule);

		//spvReflectEnumerateEntryPointDescriptorBindings

		//spvReflectEnumerateDescriptorSets
	}
};
