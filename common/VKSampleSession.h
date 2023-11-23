#pragma once
#include "FPSCounter.h"
#include "Util/Time.hpp"
#include "VKSampleBase.h"
#include "VKWindow.h"
#include <Core/IO/FileSystem.h>
#include <VKDevice.h>
#include <VkPhysicalDevice.h>
#include <VulkanCore.h>
#include <cxxopts.hpp>
#include <iostream>
#include <map>

namespace vkscommon {

	class FVDECLSPEC VKSampleSession {
	  public:
		virtual ~VKSampleSession() = default;
		virtual void run(int argc, const char **argv,
						 std::unordered_map<const char *, bool> required_device_extensions = {},
						 std::unordered_map<const char *, bool> required_instance_layers = {},
						 std::unordered_map<const char *, bool> required_instance_extensions = {}) = 0;
		virtual void customOptions([[maybe_unused]] cxxopts::OptionAdder &options) {}

		fragcore::IFileSystem *getFileSystem() const noexcept { return this->activeFileSystem; }

	  protected:
		fragcore::IFileSystem *activeFileSystem;
	};

} // namespace vkscommon