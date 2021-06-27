#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VksCommon.h"
#include "common.hpp"
#include <SDL2/SDL.h>

int main(int argc, const char **argv) {

	const int nrTransferSamples = 100;
	const int roundRobin = 3;


	const std::array<size_t, 3> memorySizes = {1024, 1024 * 1024, 1024 * 1024 * 1024};

	std::vector<double> timeSample(nrTransferSamples);
	std::vector<VkDeviceMemory> staging(roundRobin);
	std::vector<VkDeviceMemory> cpu2gpu(roundRobin);
	std::vector<VkDeviceMemory> gpu2gpu(roundRobin);
	std::vector<VkDeviceMemory> gpu2cpu(roundRobin);

	std::unordered_map<const char *, bool> required_device_extensions = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> phyDevices = core->createPhysicalDevices();
		std::shared_ptr<VKDevice> device = std::make_shared<VKDevice>(phyDevices);

		VkQueue transfer = device->getDefaultTransfer();

		//this->getLogicalDevice()->get
		const VkPhysicalDeviceMemoryProperties &
			 memProp = device->getPhysicalDevices()[0]->getMemoryProperties();
		VkDeviceSize bufferSize = 1024;


		/*	CPU->GPU	*/

		//Allocate staging device memory
		for(int i = 0; i < roundRobin; i++){
			//VKHelper::createMemory(device->getPhysicalDevice(0), bufferSize, device->getPhysicalDevice(0)->getMemoryProperties(), )
		}



		/*	GPU->GPU	*/

		/*	GPU->CPU	*/

		/*	*/
		for(int i = 0; i < memProp.memoryHeapCount; i++){
			if(memProp.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
//				VKHelper::createBuffer(getDevice(), bufferSize, memProp, VK_BUFFER_USAGE_TRANSFER_DST_BIT, memProp.memoryHeaps[i].flags, memo )
			}
		}


	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}