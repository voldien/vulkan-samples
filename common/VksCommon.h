#ifndef _VK_COMMON_SAMPLES_H_
#define _VK_COMMON_SAMPLES_H_ 1
#include "VKSampleWindow.h"
#include <Core/Math.h>
#include <VKDevice.h>
#include <VKHelper.h>

#include "VKWindow.h"

#include "Importer/IOUtil.h"

#include <fmt/core.h>
#include <stdexcept>

namespace vkscommon {

	extern std::string getShaderPath(const std::string &filepath);
}

#endif
