#ifndef _STARTUP_WINDOW_SAMPLE_H_
#define _STARTUP_WINDOW_SAMPLE_H_ 1
#include "VulkanCore.h"
#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include"SDLWindow.h"

class VKWindow : public SDLWindow {
  public:
	VKWindow(std::shared_ptr<VulkanCore> &core, int x, int y, int width, int height);
	VKWindow(const VKWindow &other) = delete;
	~VKWindow(void);

  public:
	virtual void Initialize(void);
	virtual void Release(void);
	virtual void draw(void);
	virtual void run(void);
	virtual void onResize(int width, int height);

	virtual void createLogisticDevice(VkQueueFlags queues);

  public:
  /*	Vulkan methods.	*/
	VkDevice getDevice(void) const;
	int getCurrentFrame(void) const;
	int swapChainImageCount() const;
	VkFramebuffer getDefaultFrameBuffer(void) const;
	VkFormat depthStencilFormat(void) const;
	VkImage depthStencilImage(void) const;

	/*	*/
	VkCommandBuffer getCurrentCommandBuffer(void) const;
	VkRenderPass getDefaultRenderPass(void) const;
	VkCommandPool getGraphicCommandPool(void)const;
	VkImage getDefaultImage(void) const;
	VkQueue getGraphicQueue(void) const;
	VkPhysicalDevice physicalDevice() const;
	std::vector<VkQueue> getQueues(void) const noexcept;
	std::vector<VkPhysicalDevice> getPhyiscalDevices(void);
	std::vector<VkCommandBuffer>& getCommandBuffers(void);
	std::vector<VkFramebuffer> &getFrameBuffers(void) const;
	// virtual void std::vector<SupportedExtensions> getSupportedExtensions(void);

  public:

	virtual void vsync(bool state);

	virtual void setFullScreen(bool fullscreen);

	virtual void swapBuffer(void);

  protected:
	virtual void createSwapChain(void);
	virtual void recreateSwapChain(void);
	virtual void cleanSwapChain(void);

  private:
	std::shared_ptr<VulkanCore> core;
	typedef struct _SwapchainBuffers {
		struct SwapChainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		SwapChainSupportDetails details; /*  */

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkCommandBuffer> commandBuffers;

		/*	*/
		VkFormat swapChainImageFormat;
		VkRenderPass renderPass;
		VkSwapchainKHR swapchain; /*  */
		VkExtent2D chainExtend;	  /*  */
		int currentFrame;
	} SwapchainBuffers;

	/*  */
	// VkDevice device;
	VkQueue queue; // TODO rename graphicsQueue
	VkQueue presentQueue;

	/*  */
	VkPhysicalDeviceProperties gpu_props;
	VkQueueFamilyProperties *queue_props;
	uint32_t graphics_queue_node_index;

	VkSurfaceKHR surface;

	/*  Collection of swap chain variables. */
	SwapchainBuffers *swapChain;

	/*  */
	// VkSemaphore imageAvailableSemaphore;
	// VkSemaphore renderFinishedSemaphore;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	std::vector<VkFence> imageAvailableFence;

	VkCommandPool cmd_pool;
	VkCommandPool compute_pool;
	VkCommandPool transfer_pool;
	// VkQueueFamilyIndices indices
};

#endif
