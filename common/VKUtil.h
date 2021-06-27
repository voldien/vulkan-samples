#ifndef _VKSAMPLES_VK_UTILS_H_
#define _VKSAMPLES_VK_UTILS_H_ 1
#include <fmt/format.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

static const char *getVKResultSymbol(int symbol) {
	switch (symbol) {
	case VK_SUCCESS:
		return "VK_SUCCESS";
	default:
		break;
	}
}

#define VKS_VALIDATE(x)                                                                                                \
	do {                                                                                                               \
		VkResult err = x;                                                                                              \
		if (err != VK_SUCCESS) {                                                                                       \
			throw std::runtime_error(fmt::format("{} {} {}", __FILE__, __LINE__, getVKResultSymbol(x)));               \
		}                                                                                                              \
	} while (0)

#endif