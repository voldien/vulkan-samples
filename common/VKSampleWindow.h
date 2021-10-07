#pragma once
#include "VKWindow.h"

/**
 * @brief
 *
 */
class VKSampleWindow : public VKWindow {
  public:
	VKSampleWindow(int argc, const char **argv,

				   const std::unordered_map<const char *, bool> &requested_extensions = {{"VK_KHR_swapchain", true}});
};