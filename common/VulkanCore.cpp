#include"VulkanCore.h"
#include"VKHelper.h"
#include<SDL2/SDL.h>
#include<SDL2/SDL_vulkan.h>
#include<stdexcept>
#include<cassert>
#include"common.hpp"

VulkanCore::VulkanCore(int argc, const char **argv) { Initialize(); }

void VulkanCore::Initialize(void){
	/*  Initialize video support.   */
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
		// throw std::runtime_error("Failed enumerate Vulkan extension properties");
	}

	VkResult result;
	std::vector<const char *> usedInstanceExtensionNames = {
		/*	*/
		VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME,
	};
	const std::vector<const char *> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	this->enableValidationLayers = 1;

	/*  Check for supported extensions.*/
	uint32_t extensionCount = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL));
	this->instanceExtensions.resize(extensionCount);
	/*	*/
	VK_CHECK(
		vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, instanceExtensions.data()));


	/*  Check for supported validation layers.  */
	uint32_t layerCount;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, NULL));
	std::vector<VkLayerProperties> availableLayers(layerCount);
	if (this->enableValidationLayers) {
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

		/*  Check if exists.    */
		for (uint32_t i = 0; i < availableLayers.size(); i++) {
			
		}
	}

	/*  TODO add support for loading requried extensions.   */
	// TODO improve later.
	/*  Create Vulkan window.   */
	SDL_Window *tmpWindow = SDL_CreateWindow("", 0, 0, 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
	if (tmpWindow == NULL)
		throw std::runtime_error("");

	/*	*/
	unsigned int count;
	if (!SDL_Vulkan_GetInstanceExtensions(tmpWindow, &count, NULL))
		throw std::runtime_error("SDL_Vulkan_GetInstanceExtensions");
	unsigned int additional_extension_count = (unsigned int)usedInstanceExtensionNames.size();
	usedInstanceExtensionNames.resize((size_t)(additional_extension_count + count));
	if (!SDL_Vulkan_GetInstanceExtensions(tmpWindow, &count, &usedInstanceExtensionNames[additional_extension_count]))
		throw std::runtime_error("SDL_Vulkan_GetInstanceExtensions");
	SDL_DestroyWindow(tmpWindow);

	/*  Get Vulkan version. */
	uint32_t version;
	VK_CHECK(vkEnumerateInstanceVersion(&version));

	/*	Primary Vulkan instance Object. */
	VkApplicationInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pNext = NULL;
	ai.pApplicationName = "Vulkan Sample";
	ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ai.pEngineName = "Vulkan Sample Engine";
	ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ai.apiVersion = version;

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfoExt = {
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT, // sType
		NULL,													 // pNext
		VK_DEBUG_REPORT_ERROR_BIT_EXT |							 // flags
		VK_DEBUG_REPORT_WARNING_BIT_EXT,
		NULL, // myOutputDebugString,                                        // pfnCallback
		NULL  // pUserData
	};
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = &callbackCreateInfoExt,
	};
	VkValidationCheckEXT validationCheckExt[] = {VK_VALIDATION_CHECK_ALL_EXT};
	VkValidationFlagsEXT validationFlagsExt = {.sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT,
											   .pNext = NULL, //&debugUtilsMessengerCreateInfoExt,
											   .disabledValidationCheckCount = 1,
											   .pDisabledValidationChecks = validationCheckExt};

	/*	Prepare the instance object. */
	VkInstanceCreateInfo ici = {};
	ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	if (this->enableValidationLayers)
		ici.pNext = NULL;
	else
		ici.pNext = NULL;
	ici.flags = 0;
	ici.pApplicationInfo = &ai;
	if (this->enableValidationLayers) {
		ici.enabledLayerCount = validationLayers.size();
		ici.ppEnabledLayerNames = validationLayers.data();
	} else {
		ici.enabledLayerCount = 0;
		ici.ppEnabledLayerNames = NULL;
	}
	ici.enabledExtensionCount = usedInstanceExtensionNames.size();
	ici.ppEnabledExtensionNames = usedInstanceExtensionNames.data();

	/*	Create Vulkan instance.	*/
	VK_CHECK(vkCreateInstance(&ici, NULL, &inst));

	/*	/////////////////////////////////////////*/

	/*	Get number of physical devices. */
	unsigned int nrPhysicalDevices;
	VK_CHECK(vkEnumeratePhysicalDevices(this->inst, &nrPhysicalDevices, NULL));
	/*  Get all physical devices.    */

	physicalDevices.resize(nrPhysicalDevices);
	VK_CHECK(vkEnumeratePhysicalDevices(this->inst, &nrPhysicalDevices, &this->physicalDevices[0]));

	/*	*/
	uint32_t nrPhysicalDeviceGroupCount;
	VK_CHECK(vkEnumeratePhysicalDeviceGroups(this->inst, &nrPhysicalDeviceGroupCount, NULL));

	std::vector<VkPhysicalDeviceGroupProperties> phyiscalGroupDevices(nrPhysicalDeviceGroupCount);
	VK_CHECK(vkEnumeratePhysicalDeviceGroups(this->inst, &nrPhysicalDeviceGroupCount, phyiscalGroupDevices.data()));

	/*  TODO add selection function. */
	std::vector<VkPhysicalDevice> selectedDevices;
	VKHelper::selectDefaultDevices(physicalDevices, selectedDevices);

	/*	*/
	this->gpu = selectedDevices[0];

	/*  */
	VkPhysicalDeviceFeatures supportedFeatures = {};
	vkGetPhysicalDeviceFeatures(this->gpu, &supportedFeatures);
	/*  Fetch memory properties.   */
	vkGetPhysicalDeviceMemoryProperties(this->gpu, &this->memProperties);

	/*  Select queue family.    */
	/*  TODO improve queue selection.   */
	vkGetPhysicalDeviceQueueFamilyProperties(this->gpu, &this->queue_count, NULL);

	this->queue_props = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * this->queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(this->gpu, &this->queue_count, this->queue_props);
	assert(this->queue_count >= 1);

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(this->gpu, &features);

	/*  Select queue with graphic.  */ // TODO add queue for compute and etc.
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t computeQueueNodeIndex;
	for (uint32_t i = 0; i < this->queue_count; i++) {
		if ((this->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (graphicsQueueNodeIndex == UINT32_MAX)
				graphicsQueueNodeIndex = i;
		}
		if ((this->queue_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
			if (computeQueueNodeIndex == UINT32_MAX)
				computeQueueNodeIndex = i;
		}
	}
	//TODO resolve in case compute is not supported on the graphic queue.
	this->graphics_queue_node_index = graphicsQueueNodeIndex;
	this->compute_queue_node_index = graphicsQueueNodeIndex;

	float queue_priorities[1] = {1.0};
	const VkDeviceQueueCreateInfo queue = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										   .pNext = NULL,
										   .flags = 0,
										   .queueFamilyIndex = this->graphics_queue_node_index,
										   .queueCount = 1,
										   .pQueuePriorities = queue_priorities};

	/*  Required extensions.    */
	std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,	/*	*/
		// VK_NV_RAY_TRACING_EXTENSION_NAME
		// VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
		// VK_NV_GLSL_SHADER_EXTENSION_NAME
	};

	//TODO resolve that it does if debug is enabled
	if (this->enableValidationLayers) {
		// TODO determine
		deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	}

	VkDeviceGroupDeviceCreateInfo deviceGroupDeviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
	};
	if (phyiscalGroupDevices.size() > 0) {
		if (phyiscalGroupDevices[0].physicalDeviceCount > 1) {
			deviceGroupDeviceCreateInfo.physicalDeviceCount = phyiscalGroupDevices[0].physicalDeviceCount;
			deviceGroupDeviceCreateInfo.pPhysicalDevices = phyiscalGroupDevices[0].physicalDevices;
		}
	}

	VkPhysicalDeviceShaderDrawParameterFeatures deviceShaderDrawParametersFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES,
		.pNext = NULL,
		.shaderDrawParameters = VK_TRUE};
	VkPhysicalDeviceMultiviewFeatures deviceMultiviewFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
		.pNext = &deviceShaderDrawParametersFeatures,
		.multiview = VK_TRUE,
		.multiviewGeometryShader = VK_TRUE,
		.multiviewTessellationShader = VK_TRUE};
	VkPhysicalDeviceConditionalRenderingFeaturesEXT conditionalRenderingFeaturesExt = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT,
		.pNext = &deviceMultiviewFeatures,
		.conditionalRendering = VK_TRUE,
		.inheritedConditionalRendering = VK_TRUE};

	VkDeviceCreateInfo device = {};
	VkPhysicalDeviceFeatures deviceFeatures{};
	device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device.pNext = &conditionalRenderingFeaturesExt;
	device.queueCreateInfoCount = 1;
	device.pQueueCreateInfos = &queue;
	if (this->enableValidationLayers) {
		// device.enabledLayerCount = this->enabled_layer_count;
		// device.ppEnabledLayerNames =
		// 	(const char *const *)((this->enableValidationLayers) ? this->device_validation_layers : NULL);
	} else {
		device.enabledLayerCount = 0;
	}

	/*	Enable group if supported.	*/
	if (deviceGroupDeviceCreateInfo.physicalDeviceCount > 1) {
		deviceShaderDrawParametersFeatures.pNext = &deviceGroupDeviceCreateInfo;
	}
	device.enabledExtensionCount = deviceExtensions.size();
	device.ppEnabledExtensionNames = deviceExtensions.data();
	device.pEnabledFeatures = &deviceFeatures;

	/*  Create device.  */
	VK_CHECK(vkCreateDevice(this->gpu, &device, NULL, &this->device));

	/*  Get all queues.    */
	vkGetDeviceQueue(this->device, this->graphics_queue_node_index, 0, &this->graphicsQueue);
	vkGetDeviceQueue(this->device, this->graphics_queue_node_index, 0, &this->presentQueue);
	vkGetDeviceQueue(this->device, this->compute_queue_node_index, 0, &this->computeQueue);
}

VulkanCore::~VulkanCore(void){

	if (device)
		vkDestroyDevice(device, NULL);

	if (inst)
		vkDestroyInstance(inst, NULL);
}