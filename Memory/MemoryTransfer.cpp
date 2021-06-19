#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VksCommon.h"
#include "common.hpp"
#include <SDL2/SDL.h>
#include <VKWindow.h>

class MemoryTransferWindow : public VKWindow {
  private:
	std::vector<VkDeviceMemory> localMemory;
	std::vector<VkDeviceMemory> hostMemory;
	std::vector<void *> mapMemory;
	long ntime;
  public:
	MemoryTransferWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		ntime = SDL_GetPerformanceCounter();
	}
	~MemoryTransferWindow(void) {}

	virtual void Release(void) override {

	//	vkDestroyBuffer(getDevice(), vertexBuffer, nullptr);
	//	vkFreeMemory(getDevice(), vertexMemory, nullptr);

	}


	virtual void Initialize(void) override {

		//this->getLogicalDevice()->get
		const VkPhysicalDeviceMemoryProperties &
			 memProp = this->getLogicalDevice()->getPhysicalDevices()[0]->getMemoryProperties();
		VkDeviceSize bufferSize = 1024;

		for(int i = 0; i < memProp.memoryHeapCount; i++){
			if(memProp.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
//				VKHelper::createBuffer(getDevice(), bufferSize, memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT, memProp.memoryHeaps[i].flags, memo )
			}
		}

		// uniformBuffers.resize(getSwapChainImageCount());
		// uniformBuffersMemory.resize(getSwapChainImageCount());

		// VkPhysicalDeviceMemoryProperties memProperties;
		// vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

		// for (size_t i = 0; i < getSwapChainImageCount(); i++) {
		// 	VKHelper::createBuffer(getDevice(), bufferSize, memProperties,
		// 						   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		// 						   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
		// 							   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		// 						   uniformBuffers[i], uniformBuffersMemory[i]);
		// 	void *_data;
		// 	VK_CHECK(vkMapMemory(getDevice(), uniformBuffersMemory[i], 0, (size_t)sizeof(this->mvp), 0, &_data));
		// 	mapMemory.push_back(_data);
		// }


		// VkBufferCreateInfo bufferInfo = {};
		// bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		// bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		// bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		// bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// VK_CHECK(vkCreateBuffer(getDevice(), &bufferInfo, nullptr, &vertexBuffer));

		// VkMemoryRequirements memRequirements;
		// vkGetBufferMemoryRequirements(getDevice(), vertexBuffer, &memRequirements);

		// VkMemoryAllocateInfo allocInfo = {};
		// allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		// allocInfo.allocationSize = memRequirements.size;
		// allocInfo.memoryTypeIndex =
		// 	VKHelper::findMemoryType(physicalDevice(), memRequirements.memoryTypeBits,
		// 							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		// 		.value();

		// VK_CHECK(vkAllocateMemory(getDevice(), &allocInfo, nullptr, &vertexMemory));

		// VK_CHECK(vkBindBufferMemory(getDevice(), vertexBuffer, vertexMemory, 0));

		// void *data;
		// VK_CHECK(vkMapMemory(getDevice(), vertexMemory, 0, bufferInfo.size, 0, &data));
		// memcpy(data, vertices.data(), (size_t)bufferInfo.size);
		// vkUnmapMemory(getDevice(), vertexMemory);

		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		ntime = SDL_GetPerformanceCounter();

		VK_CHECK(vkQueueWaitIdle(getDefaultGraphicQueue()));

		for (int i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

			/*	Transfer the new data 	*/
			// vkCmdTran

			// vkCmdUpdateBuffer(cmd, uniformBuffers[i], 0, sizeof(mvp), &mvp);

			// VkBufferMemoryBarrier ub_barrier = {
			// 	.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			// 	.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			// 	.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			// 	.buffer = uniformBuffers[i],
			// 	.offset = 0,
			// 	.size = sizeof(mvp),
			// };
			// ub_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			// ub_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;

			// vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL,
			// 1, 					 &ub_barrier, 0, NULL);

			//vkCmdCopyBuffer(cmd, src, dst, 1, nullptr);

			//vkCmdWriteTimestamp();

			//			vkCmdEndRenderPass(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));
		}
	}

	virtual void draw(void) override {

		float elapsedTime = ((float)(SDL_GetPerformanceCounter() - ntime) / (float)SDL_GetPerformanceFrequency());

		// printf("%f\n", elapsedTime);
		// this->mvp.model = glm::mat4(1.0f);
		// this->mvp.view = glm::mat4(1.0f);
		// this->mvp.view = glm::translate(this->mvp.view, glm::vec3(0, 0, -5));
		// this->mvp.model = glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45), glm::vec3(0.0f, 1.0f, 0.0f));
		// this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));

		// // Setup the range
		// memcpy(mapMemory[getCurrentFrame()], &mvp, (size_t)sizeof(this->mvp));
		// VkMappedMemoryRange stagingRange = {.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		// 									.memory = uniformBuffersMemory[getCurrentFrame()],
		// 									.offset = 0,
		// 									.size = (size_t)sizeof(this->mvp)};
		// vkFlushMappedMemoryRanges(getDevice(), 1, &stagingRange);
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>(argc, argv);
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		printf("%s\n", devices[0]->getDeviceName());
		std::shared_ptr<VKDevice> device = std::make_shared<VKDevice>(devices);
		MemoryTransferWindow window(core, device);

		window.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}