#ifndef _STARTUP_WINDOW_SAMPLE_H_
#define _STARTUP_WINDOW_SAMPLE_H_ 1
#include "SDLWindow.h"
#include "VKDevice.h"
#include "VulkanCore.h"
#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include<iostream>
#include <vulkan/vulkan.h>

class VKWindow : public SDLWindow {
  public:
	/**
	 * @brief Construct a new VKWindow object
	 *
	 * @param core
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 */
	// VKWindow(std::shared_ptr<VulkanCore> &core, int x, int y, int width, int height);

	/**
	 * @brief Construct a new VKWindow object
	 *
	 * @param core
	 * @param device
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 */
	VKWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device, int x, int y, int width, int height);
	VKWindow(const VKWindow &other) = delete;
	~VKWindow(void);

  public:
	/**
	 * @brief
	 *
	 */
	virtual void Initialize(void);
	/**
	 * @brief
	 *
	 */
	virtual void Release(void);
	/**
	 * @brief
	 *
	 */
	virtual void draw(void);
	/**
	 * @brief
	 *
	 */
	virtual void run(void);
	// virtual void run(VkCommandBuffer cmdBuffer);
	/**
	 * @brief
	 *
	 */
	virtual void onResize(int width, int height);

	// virtual void createLogisticDevice(VkQueueFlags queues);

  public:
	/*	Vulkan methods.	*/
	VkDevice getDevice(void) const;
	uint32_t getCurrentFrame(void) const;
	uint32_t swapChainImageCount() const;
	VkFramebuffer getDefaultFrameBuffer(void) const;
	VkFormat depthStencilFormat(void) const;
	VkImage depthStencilImage(void) const;

	/*	*/
	VkCommandBuffer getCurrentCommandBuffer(void) const;
	VkRenderPass getDefaultRenderPass(void) const;
	VkCommandPool getGraphicCommandPool(void) const;
	//VkCommandPool getComputeCommandPool(void) const noexcept;
	VkImage getDefaultImage(void) const;
	VkImageView getDefaultImageView(void) const;
	VkQueue getDefaultGraphicQueue(void) const;
	VkFormat getDefaultImageFormat(void) const { return this->swapChain->swapChainImageFormat; }

	std::vector<VkImage> getSwapChainImages(void) const noexcept { return this->swapChain->swapChainImages; }

	const std::shared_ptr<VKDevice> &getLogicalDevice(void) const noexcept { return this->device; }

	VkPhysicalDevice physicalDevice() const;
	std::vector<VkQueue> getQueues(void) const noexcept;
	std::vector<VkPhysicalDevice> getPhyiscalDevices(void);

	std::vector<VkCommandBuffer> &getCommandBuffers(void) const noexcept;
	std::vector<VkFramebuffer> &getFrameBuffers(void) const noexcept;
	// virtual void std::vector<SupportedExtensions> getSupportedExtensions(void);

  public:
	virtual void vsync(bool state);

	virtual void setFullScreen(bool fullscreen);

	virtual void swapBuffer(void);

  protected:	/*	Internal method for creating swapchains.	*/
	virtual void createSwapChain(void);
	virtual void recreateSwapChain(void);
	virtual void cleanSwapChain(void);

  private:
	std::shared_ptr<VKDevice> device;
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

		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		/*	*/
		VkFormat swapChainImageFormat;
		VkRenderPass renderPass;
		VkSwapchainKHR swapchain; /*  */
		VkExtent2D chainExtend;	  /*  */
		int currentFrame;
		bool vsync = false;
	} SwapchainBuffers;

	/*  */
	// VkDevice device;
	VkQueue queue; // TODO rename graphicsQueue
	VkQueue presentQueue;

	/*  */
	uint32_t graphics_queue_node_index;

	VkSurfaceKHR surface;

	/*  Collection of swap chain variables. */
	SwapchainBuffers *swapChain;	//TODO remove as pointer

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
};

#endif
