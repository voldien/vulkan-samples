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
#include "DataStructure/PoolAllocator.h"
#include "VKDataStructure.h"
#include "VKSampleBase.h"
#include "vulkan/vulkan_core.h"
#include <IO/IOUtil.h>
#include <ShaderCompiler.h>

namespace vksample {

	/**
	 * @brief
	 *
	 */
	// TODO: refractor
	class FVDECLSPEC ShaderLoader : public fragcore::Object {
	  public:
		ShaderLoader(VKSampleSessionBase &renderBackend);
		// TODO: add options.
		/**
		 * @brief
		 *
		 */
		GraphicPipeline *loadGraphicProgram(const std::vector<uint32_t> *vertex, const std::vector<uint32_t> *fragment,
											const std::vector<uint32_t> *geometry = nullptr,
											const std::vector<uint32_t> *tesselationc = nullptr,
											const std::vector<uint32_t> *tesselatione = nullptr,
											const void *pNext = nullptr); // TODO: pnext
		// TODO: add options.
		/**
		 * @brief
		 *
		 */
		ComputePipeline loadComputeProgram(const std::vector<uint32_t> *computeBinary,
										   const void *pNext = nullptr); // TODO: pnext

		/**
		 * @brief
		 *
		 */
		GraphicPipeline loadMeshProgram(const std::vector<uint32_t> *meshs, const std::vector<uint32_t> *tasks,
										const std::vector<uint32_t> *fragment);

		RayTracingPipeline loadRayTracingProgram(const std::vector<uint32_t> *anyHitBinary,
												 const std::vector<uint32_t> *closestHitBinary,
												 const std::vector<uint32_t> *generalHitBinary,
												 const std::vector<uint32_t> *genIntersectionBinary,
												 const std::vector<uint32_t> *genRayBinary,
												 const std::vector<uint32_t> *genMissBinary);

		VkPipelineCache getPipelineCache() const noexcept { return this->pipelineCache; }

	  private:
		int loadShader(const std::vector<char> &data, const int type);

	  private:
		fragcore::PoolAllocator<GraphicPipeline> graphic;
		fragcore::PoolAllocator<ComputePipeline> compute;
		fragcore::PoolAllocator<RayTracingPipeline> raytracing;
		VkPipelineCache pipelineCache;
		VKSampleSessionBase &renderer;
	};
} // namespace vksample
