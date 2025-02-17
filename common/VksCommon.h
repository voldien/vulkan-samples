#pragma once
#include "VKSample.h"
#include "VKWindow.h"
#include <Math/Math.h>
#include <Util/IOUtil.h>
#include <Util/PipelineLayoutUtil.h>
#include <VKDevice.h>
#include <VKHelper.h>

#include <fmt/core.h>
#include <stdexcept>

namespace vksample {

	extern std::string getShaderPath(const std::string &filepath);
}
