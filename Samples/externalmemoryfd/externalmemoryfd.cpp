#include "Importer/ImageImport.h"
#include "VKUtil.h"
#include "VksCommon.h"
#include <cxxopts.hpp>
#include <fmt/format.h>

class ExternalMemoryFD : public VKSampleSession {
  public:
	ExternalMemoryFD(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKSampleSession(core, device) {}

	virtual ~ExternalMemoryFD() { this->release(); }

	virtual void release() override {
		vkFreeMemory(this->getDevice(), memory, nullptr);
		vkDestroyBuffer(this->getDevice(), buffer, nullptr);
	}

	virtual void run() override {

		const VkDeviceSize bufferSize = sizeof(float) * 1024 * 1024;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VKS_VALIDATE(vkCreateBuffer(this->getDevice(), &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(this->getDevice(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =
			VKHelper::findMemoryType(this->getPhysicalDevice()->getHandle(), memRequirements.memoryTypeBits,
									 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
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
		vkMemoryGetFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

		VKS_VALIDATE(vkGetMemoryFdKHR(this->getDevice(), &vkMemoryGetFdInfo, &fd));

		// FILE *bufferFD = fdopen(fd, "r");
		void *buffer = malloc(bufferSize);

		ssize_t readResult; // = fread(buffer, bufferSize, 1, bufferFD);
		readResult = read(fd, buffer, bufferSize);
		if (readResult != bufferSize) {
			throw cxxexcept::RuntimeException("Could not read the whole buffer {}, read {} {}", bufferSize, readResult,
											  strerror(errno));
		}
		close(fd);

		VkMemoryFdPropertiesKHR prop = {.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR, .pNext = nullptr};

		VKS_VALIDATE(
			vkGetMemoryFdPropertiesKHR(this->getDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT, fd, &prop));

		//	prop.memoryTypeBits

		/**/

		// VkMemoryDedicatedAllocateInfoKHR dedicated_memory_info = {
		// 	.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
		// 	.image = image,
		// };
		// VkImportMemoryFdInfoKHR import_memory_info = {
		// 	.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
		// 	.pNext = &dedicated_memory_info,
		// 	.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
		// 	.fd = fd,
		// };
		// VkMemoryAllocateInfo alloc_info = {
		// 	.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		// 	.pNext = &import_memory_info,
		// 	.allocationSize = memRequirements.size,
		// 	.memoryTypeIndex = allocInfo.memoryTypeIndex,
		// };
		// VkDeviceMemory allocmem;
		// VKS_VALIDATE(vkAllocateMemory(getDevice(), &alloc_info, nullptr, &allocmem));
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
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}