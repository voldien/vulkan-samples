#ifndef _VKSAMPLES_VK_UTILS_H_
#define _VKSAMPLES_VK_UTILS_H_ 1
#include <fmt/format.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

static const char *getVKResultSymbol(int symbol) {
	switch (symbol) {
	case VK_SUCCESS:
		return "VK_SUCCESS";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST:
		return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
	case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
		return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR";
	default:
		return "";
	}
}

#define VKS_VALIDATE(x)                                                                                                \
	do {                                                                                                               \
		VkResult err = x;                                                                                              \
		if (err != VK_SUCCESS) {                                                                                       \
			throw std::runtime_error(fmt::format("{} {} {} - {}", __FILE__, __LINE__, x, getVKResultSymbol(x)));       \
		}                                                                                                              \
	} while (0)

#endif