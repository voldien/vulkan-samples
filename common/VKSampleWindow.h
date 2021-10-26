#pragma once
#include "VKWindow.h"
#include <map>

/**
 * @brief
 *
 */
class VKSampleWindow : public VKWindow {
  public:
	VKSampleWindow(int argc, const char **argv, std::unordered_map<const char *, bool> required_device_extensions = {},
				   std::unordered_map<const char *, bool> required_instance_layers = {},
				   std::unordered_map<const char *, bool> required_instance_extensions = {});
};
