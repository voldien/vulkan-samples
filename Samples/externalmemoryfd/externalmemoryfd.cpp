#include "Importer/ImageImport.h"
#include "VKUtil.h"
#include "VksCommon.h"
#include <cxxopts.hpp>
#include <fmt/format.h>

class ExternalMemoryFD : public VKSampleSession {
  public:
	ExternalMemoryFD(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKSampleSession(core, device) {}

	virtual void release() {
		vkFreeMemory(this->getDevice(), memory, nullptr);
		vkDestroyBuffer(this->getDevice(), buffer, nullptr);
	}

	virtual void run() override {

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(float) * 2048;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VKS_VALIDATE(vkCreateBuffer(this->getDevice(), &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(this->getDevice(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =
			VKHelper::findMemoryType(this->physicalDevice(), memRequirements.memoryTypeBits,
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				.value();

		VKS_VALIDATE(vkAllocateMemory(this->getDevice(), &allocInfo, nullptr, &memory));

		VKS_VALIDATE(vkBindBufferMemory(this->getDevice(), buffer, memory, 0));

		PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR =
			(PFN_vkGetMemoryFdKHR)vkGetInstanceProcAddr(this->getInstance(), "vkGetMemoryFdKHR");
		PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR =
			(PFN_vkGetMemoryFdPropertiesKHR)vkGetInstanceProcAddr(this->getInstance(), "vkGetMemoryFdPropertiesKHR");

		int fd;
		VkMemoryGetFdInfoKHR vkMemoryGetFdInfo{};
		vkMemoryGetFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
		vkMemoryGetFdInfo.pNext = nullptr;
		vkMemoryGetFdInfo.memory = memory;
		vkMemoryGetFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

		VKS_VALIDATE(vkGetMemoryFdKHR(this->getDevice(), &vkMemoryGetFdInfo, &fd));

		VkMemoryFdPropertiesKHR prop;

		VKS_VALIDATE(
			vkGetMemoryFdPropertiesKHR(this->getDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT, fd, &prop));
	}

  protected:
	VkDeviceMemory memory;
	VkBuffer buffer;
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {{VK_KHR_SURFACE_EXTENSION_NAME, false},
																		   {"VK_KHR_xlib_surface", false}};
	std::unordered_map<const char *, bool> required_device_extensions = {
		{VK_KHR_SWAPCHAIN_EXTENSION_NAME, false},
		{VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, true},
	};

	try {
		VKSampleWindow<ExternalMemoryFD> sample(argc, argv, required_device_extensions, {},
												required_instance_extensions);
		sample.run();

	} catch (const std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}