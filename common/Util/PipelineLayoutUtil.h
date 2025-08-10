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
#include <optional>
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

		using InputLayout = struct input_layout_t {
			std::vector<VkVertexInputAttributeDescription> inputAttributes;
			std::vector<VkVertexInputBindingDescription> inputBindings;
		};
		using OutputLayout = struct output_layout_t {
			VkPipelineVertexInputStateCreateInfo vertexInput;
		};

		using InputOutputLayout = struct input_output_layout_t {
			InputLayout input;
			InputLayout output;
		};

		static InputLayout getInputLayout(const std::vector<uint32_t> &moduleData) {
			/*	*/
			InputLayout ioLayout;

			SpvReflectShaderModule spvModule = getReflectModule(moduleData);

			SpvReflectResult result;

			uint32_t count = 0;
			spvReflectEnumerateInputVariables(&spvModule, &count, nullptr);
			std::vector<SpvReflectInterfaceVariable *> variables(count);
			spvReflectEnumerateInputVariables(&spvModule, &count, variables.data());
			int stride_offset = 0;
			for (size_t i_set = 0; i_set < variables.size(); ++i_set) {

				// variables[i_set]->type_description
				VkVertexInputBindingDescription binding;
				VkVertexInputAttributeDescription attributes;

				attributes.binding = 0;
				attributes.location = variables[i_set]->location;
				attributes.format = static_cast<VkFormat>(variables[i_set]->format);
				attributes.offset = stride_offset;

				ioLayout.inputAttributes.push_back(attributes);
				ioLayout.inputBindings.push_back(binding);

				stride_offset += 4 * 4;
			}

			return ioLayout;
		}

		static std::vector<DescriptorSetLayoutData> getDefaultBinding(const std::vector<uint32_t> &moduleData,
																	  const char *entryPoint = "main") {

			/*	*/
			SpvReflectShaderModule spvModule = getReflectModule(moduleData);
			SpvReflectResult result;

			/*	*/
			uint32_t count = 0;
			result = spvReflectEnumerateDescriptorSets(&spvModule, &count, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw cxxexcept::RuntimeException("Failed to get SPV Reflect Descriptor Set Count: {0}",
												  magic_enum::enum_name(result));
			}

			/*	*/
			std::vector<SpvReflectDescriptorSet *> sets(count);
			result = spvReflectEnumerateDescriptorSets(&spvModule, &count, sets.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw cxxexcept::RuntimeException("Failed to get SPV Reflect Descriptor Set: {0}",
												  magic_enum::enum_name(result));
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

				// spvReflectEnumerateDescriptorSets
			}

			/*	*/
			spvReflectDestroyShaderModule(&spvModule);

			return set_layouts;
		}

		static std::optional<std::array<size_t, 3>> getLocalSize(const std::vector<uint32_t> &moduleData,
																 const char *entryPoint = "main") {

			/*	*/
			SpvReflectShaderModule spvModule = getReflectModule(moduleData);

			const SpvReflectEntryPoint *moduleEntryPoint = spvReflectGetEntryPoint(&spvModule, entryPoint);
			if (moduleEntryPoint) {

				return {
					{moduleEntryPoint->local_size.x, moduleEntryPoint->local_size.y, moduleEntryPoint->local_size.z}};
			}
			return {};
		}

		template <typename T>
		static SpvReflectShaderModule getReflectModule(const std::vector<T> &moduleData,
													   SpvReflectShaderModule *module = nullptr) {

			SpvReflectShaderModule spvModule;
			SpvReflectResult result =
				spvReflectCreateShaderModule(moduleData.size() * sizeof(moduleData[0]), moduleData.data(), &spvModule);

			/*	*/
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw cxxexcept::RuntimeException("Failed to create SPV Reflect: {0}", magic_enum::enum_name(result));
			}
			return spvModule;
		}
	};

} // namespace vksample