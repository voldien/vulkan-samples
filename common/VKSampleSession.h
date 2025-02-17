/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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
#include <IO/FileSystem.h>
#include <VKDevice.h>
#include <VkPhysicalDevice.h>
#include <VulkanCore.h>
#include <cxxopts.hpp>

namespace vksample {

	class FVDECLSPEC VKSampleSession {
	  public:
		virtual ~VKSampleSession() = default;
		virtual void run(int argc, const char **argv,
						 std::unordered_map<const char *, bool> required_device_extensions = {},
						 std::unordered_map<const char *, bool> required_instance_layers = {},
						 std::unordered_map<const char *, bool> required_instance_extensions = {}) = 0;

		virtual void customOptions([[maybe_unused]] cxxopts::OptionAdder &options) {}

		cxxopts::ParseResult &getResult() noexcept { return this->parseResult; }
		void setCommandResult(cxxopts::ParseResult &result) noexcept { this->parseResult = result; }

		fragcore::IFileSystem *getFileSystem() const noexcept { return this->activeFileSystem; }

	  protected:
		fragcore::IFileSystem *activeFileSystem;
		cxxopts::ParseResult parseResult;
	};

} // namespace vksample