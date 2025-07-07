/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Exception.hpp"
#include "magic_enum.hpp"
#include "vulkan/vulkan_core.h"
#include <cassert>
#include <spirv-reflect/spirv_reflect.h>

namespace vksample {

	using namespace spv_reflect;

	class PipelineLayoutUtil {
	  public:

		struct DescriptorSetLayoutData {
			uint32_t set_number;
			VkDescriptorSetLayoutCreateInfo create_info;
			std::vector<VkDescriptorSetLayoutBinding> bindings;
		};

		static std::vector<DescriptorSetLayoutData> getDefaultBinding(const std::vector<uint32_t> &moduleData) {

			/*	*/
			SpvReflectShaderModule spvModule;
			SpvReflectResult result = spvReflectCreateShaderModule(moduleData.size() * sizeof(moduleData[0]), moduleData.data(), &spvModule);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw cxxexcept::RuntimeException("Failed to create SPV Reflect: {0}", magic_enum::enum_name(result));
			}

			/*	*/
			uint32_t count = 0;
			result = spvReflectEnumerateDescriptorSets(&spvModule, &count, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw cxxexcept::RuntimeException("Failed to get SPV Reflect Descriptor Set Count: {0}", magic_enum::enum_name(result));
			}

			/*	*/
			std::vector<SpvReflectDescriptorSet *> sets(count);
			result = spvReflectEnumerateDescriptorSets(&spvModule, &count, sets.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw cxxexcept::RuntimeException("Failed to get SPV Reflect Descriptor Set: {0}", magic_enum::enum_name(result));
			}

			/*	*/
			std::vector<DescriptorSetLayoutData> set_layouts(count);
			for (size_t i_set = 0; i_set < sets.size(); ++i_set) {
				
				/*	*/
				const SpvReflectDescriptorSet &refl_set = *(sets[i_set]);
				DescriptorSetLayoutData &layout = set_layouts[i_set];

				layout.bindings.resize(refl_set.binding_count);

				/*	*/
				for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {

					const SpvReflectDescriptorBinding &refl_binding = *(refl_set.bindings[i_binding]);
					VkDescriptorSetLayoutBinding &layout_binding = layout.bindings[i_binding];

					/*	*/
					layout_binding.binding = refl_binding.binding;
					layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
					layout_binding.descriptorCount = 1;
					for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
						layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
					}
					layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(spvModule.shader_stage);
				}

				/*	*/
				layout.set_number = refl_set.set;
				layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layout.create_info.bindingCount = refl_set.binding_count;
				layout.create_info.pBindings = layout.bindings.data();

				/*	*/
				// spvReflectEnumerateEntryPointDescriptorBindings
				// spvReflectEnumerateDescriptorSets
			}

			/*	*/
			spvReflectDestroyShaderModule(&spvModule);

			return set_layouts;
		}
	};

} // namespace vksample