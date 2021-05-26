#include "VkPhysicalDevice.h"
#include "VKHelper.h"

PhysicalDevice::PhysicalDevice(const std::shared_ptr<VulkanCore> &core, VkPhysicalDevice device) {
	PhysicalDevice(core->getHandle(), device);
	vkCore = core;
}

PhysicalDevice::PhysicalDevice(VkInstance instance, VkPhysicalDevice device) {

	/*  Get feature of the device.  */
	vkGetPhysicalDeviceFeatures(device, &this->features);

	/*  Get memory properties.   */
	vkGetPhysicalDeviceMemoryProperties(device, &this->memProperties);

	/*	Get device properties.	*/
	vkGetPhysicalDeviceProperties(device, &this->properties);

	/*  Select queue family.    */
	/*  TODO improve queue selection.   */
	uint32_t nrQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &nrQueueFamilies, VK_NULL_HANDLE);
	this->queueFamilyProperties.resize(nrQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &nrQueueFamilies, this->queueFamilyProperties.data());

	this->mdevice = device;
}

bool PhysicalDevice::isPresentable(VkSurfaceKHR surface, uint32_t queueFamilyIndex) const noexcept {
	VkBool32 present_supported{VK_FALSE};

	if (surface != VK_NULL_HANDLE) {
		vkGetPhysicalDeviceSurfaceSupportKHR(this->getHandle(), queueFamilyIndex, surface, &present_supported);
	}

	return present_supported;
}