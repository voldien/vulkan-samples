#include "VulkanCore.h"
#include "VKHelper.h"
#include "VkPhysicalDevice.h"
#include "common.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <getopt.h>
#include <stdexcept>

VulkanCore::VulkanCore(int argc, const char **argv, const std::unordered_map<const char *, bool> &requested_extensions,
					   const std::unordered_map<const char *, bool> &requested_layers) {
	Initialize(requested_extensions, requested_layers);
}

static VkBool32 myDebugReportCallbackEXT(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
										 uint64_t object, size_t location, int32_t messageCode,
										 const char *pLayerPrefix, const char *pMessage, void *pUserData) {
	return VK_TRUE;
}

void VulkanCore::Initialize(const std::unordered_map<const char *, bool> &requested_extensions,
							const std::unordered_map<const char *, bool> &requested_layers) {
	/*  Initialize video support.   */
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error(fmt::format("Failed init SDL video: {}", SDL_GetError()));
	}

	VkResult result;
	std::vector<const char *> usedInstanceExtensionNames = {
		/*	*/
		VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME,
	};

	/*	*/
	std::vector<const char *> validationLayers;
	validationLayers.reserve(validationLayers.size() + requested_layers.size());
	for (const std::pair<const char *, bool> &n : requested_layers) {
		// TODO add logic to determine if supported.
		if (n.second) {
			validationLayers.push_back(n.first);
			this->useValidationLayers = true;
		}
	}

	/*  Check for supported extensions.*/
	uint32_t extensionCount = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &extensionCount, VK_NULL_HANDLE));
	this->instanceExtensions.resize(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &extensionCount, instanceExtensions.data()));

	/*  Check for supported validation layers.  */
	uint32_t layerCount;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE));
	instanceLayers.resize(layerCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, instanceLayers.data()));
	if (this->useValidationLayers) {
		/*  Check if exists.    */
		for (uint32_t i = 0; i < instanceLayers.size(); i++) {
			
		}
	}

	/*  TODO add support for loading requried extensions.   */
	// TODO improve later.
	/*  Create Vulkan window.   */
	SDL_Window *tmpWindow = SDL_CreateWindow("", 0, 0, 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
	if (tmpWindow == NULL)
		throw std::runtime_error(fmt::format("Failed to create Tmp Vulkan window - {}", SDL_GetError()));
	/*	*/
	unsigned int count;
	if (!SDL_Vulkan_GetInstanceExtensions(tmpWindow, &count, nullptr))
		throw std::runtime_error("SDL_Vulkan_GetInstanceExtensions");
	unsigned int additional_extension_count = (unsigned int)usedInstanceExtensionNames.size();
	usedInstanceExtensionNames.resize((size_t)(additional_extension_count + count));
	if (!SDL_Vulkan_GetInstanceExtensions(tmpWindow, &count, &usedInstanceExtensionNames[additional_extension_count]))
		throw std::runtime_error("SDL_Vulkan_GetInstanceExtensions");
	SDL_DestroyWindow(tmpWindow);

	/*  Get Latest Vulkan version. */
	uint32_t version;
	VK_CHECK(vkEnumerateInstanceVersion(&version));

	/*	Primary Vulkan instance Object. */
	VkApplicationInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pNext = VK_NULL_HANDLE;
	ai.pApplicationName = "Vulkan Sample";
	ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ai.pEngineName = "Vulkan Sample Engine";
	ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ai.apiVersion = version;

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfoExt = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT, // sType
		.pNext = NULL,													  // pNext
		.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |						  // flags
				 VK_DEBUG_REPORT_WARNING_BIT_EXT,
		.pfnCallback =
			&myDebugReportCallbackEXT, // myOutputDebugString,                                        // pfnCallback
		.pUserData = VK_NULL_HANDLE			   // pUserData
	};
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = &callbackCreateInfoExt,
	};
	VkValidationCheckEXT validationCheckExt[] = {VK_VALIDATION_CHECK_ALL_EXT};
	VkValidationFlagsEXT validationFlagsExt = {.sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT,
											   .pNext = VK_NULL_HANDLE, //&debugUtilsMessengerCreateInfoExt,
											   .disabledValidationCheckCount = 1,
											   .pDisabledValidationChecks = validationCheckExt};

	/*	Prepare the instance object. */
	VkInstanceCreateInfo ici = {};
	ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	if (this->useValidationLayers)
		ici.pNext = &callbackCreateInfoExt;
	else
		ici.pNext = NULL;
	ici.flags = 0;
	ici.pApplicationInfo = &ai;
	if (this->useValidationLayers) {
		ici.enabledLayerCount = validationLayers.size();
		ici.ppEnabledLayerNames = validationLayers.data();
	} else {
		ici.enabledLayerCount = 0;
		ici.ppEnabledLayerNames = VK_NULL_HANDLE;
	}
	ici.enabledExtensionCount = usedInstanceExtensionNames.size();
	ici.ppEnabledExtensionNames = usedInstanceExtensionNames.data();

	/*	Create Vulkan instance.	*/
	VK_CHECK(vkCreateInstance(&ici, VK_NULL_HANDLE, &inst));

	/*	/////////////////////////////////////////*/

	/*	Get number of physical devices. */
	unsigned int nrPhysicalDevices;
	VK_CHECK(vkEnumeratePhysicalDevices(this->inst, &nrPhysicalDevices, VK_NULL_HANDLE));
	/*  Get all physical devices.    */

	physicalDevices.resize(nrPhysicalDevices);
	VK_CHECK(vkEnumeratePhysicalDevices(this->inst, &nrPhysicalDevices, &this->physicalDevices[0]));

	/*	*/
	uint32_t nrPhysicalDeviceGroupCount;
	VK_CHECK(vkEnumeratePhysicalDeviceGroups(this->inst, &nrPhysicalDeviceGroupCount, VK_NULL_HANDLE));

	std::vector<VkPhysicalDeviceGroupProperties> phyiscalGroupDevices(nrPhysicalDeviceGroupCount);
	VK_CHECK(vkEnumeratePhysicalDeviceGroups(this->inst, &nrPhysicalDeviceGroupCount, phyiscalGroupDevices.data()));

	/*  TODO add selection function. */
	std::vector<VkPhysicalDevice> selectedDevices;
	VKHelper::selectDefaultDevices(physicalDevices, selectedDevices);
}

void VulkanCore::parseOptions(int argc, const char **argv) {

	/*	TODO reorder    */
	static const char *shortarg = "vVdqh"
								  "D:";
	static struct option longoptions[] = {
		/*  First pass arguments.   */
		{"version", no_argument, NULL, 'v'}, /*	Print version of application.	*/
		{"verbose", no_argument, NULL, 'V'}, /*	Print.	*/
		{"debug", no_argument, NULL, 'd'},	 /*	Debug.	*/
		{"quite", no_argument, NULL, 'q'},	 /*	Quite .	*/
		{"help", no_argument, NULL, 'h'},	 /*	Help.	*/
		{"device-index", no_argument, NULL, 'D'},
		/*  Texture arguments.  16 texture unit support by default. */
		{"texture0", required_argument, NULL, 't'}, /*	Texture on index 0. */

		{NULL, 0, NULL, 0},
	};

	int c, index;
	while ((c = getopt_long(argc, (char *const *)argv, shortarg, longoptions, &index)) != EOF) {
		const char *option = NULL;
		if (index >= 0 && index < sizeof(longoptions) / sizeof(longoptions[0]))
			option = longoptions[index].name;
	}

	/*	Reset getopt.	*/
	optind = 0;
	optopt = 0;
	opterr = 0;
}

std::vector<PhysicalDevice *> VulkanCore::createPhysicalDevices(void) const {
	std::vector<PhysicalDevice *> _physicalDevices(getPhysicalDevices().size());
	for (int i = 0; i < getPhysicalDevices().size(); i++) {
		_physicalDevices[i] = createPhysicalDevice(i);
	}
	return _physicalDevices;
}

PhysicalDevice *VulkanCore::createPhysicalDevice(unsigned int index) const {
	return new PhysicalDevice(getHandle(), getPhysicalDevices()[index]);
}

VulkanCore::~VulkanCore(void) {

	// if (device)
	// 	vkDestroyDevice(device, NULL);

	if (inst)
		vkDestroyInstance(inst, NULL);
}