#ifndef _STARTUP_WINDOW_SAMPLE_H_
#define _STARTUP_WINDOW_SAMPLE_H_ 1
#include "SDLWindow.h"
#include "VKDevice.h"
#include "VulkanCore.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <vector>
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
	VkDevice getDevice(void) const noexcept;
	/*	*/
	uint32_t getCurrentFrame(void) const noexcept;
	uint32_t getSwapChainImageCount() const noexcept;
	VkFramebuffer getDefaultFrameBuffer(void) const noexcept;
	VkRenderPass getDefaultRenderPass(void) const noexcept;
	const std::vector<VkFramebuffer> &getFrameBuffers(void) const noexcept;
	/*	*/
	VkFormat depthStencilFormat(void) const noexcept;
	VkImage depthStencilImage(void) const noexcept;
	VkImageView depthStencilImageView(void) const noexcept;

	/*	*/
	VkImage getDefaultMSSAColorImage(void) const noexcept;
	VkImageView getDefaultMSSAColorImageView(void) const noexcept;

	/*	*/
	VkImage getDefaultImage(void) const;
	VkImageView getDefaultImageView(void) const;
	VkFormat getDefaultImageFormat(void) const noexcept{ return this->swapChain->swapChainImageFormat; }

	/*	*/
	VkCommandBuffer getCurrentCommandBuffer(void) const noexcept;
	const std::vector<VkCommandBuffer> &getCommandBuffers(void) const noexcept;
	VkCommandPool getGraphicCommandPool(void) const noexcept;

	// VkCommandPool getComputeCommandPool(void) const noexcept;
	const VkPhysicalDeviceProperties &physicalDeviceProperties() const noexcept;

	/*	*/
	uint32_t getGraphicQueueIndex(void) const;
	VkQueue getDefaultGraphicQueue(void) const;
	VkQueue getDefaultComputeQueue(void) const;

	const std::vector<VkImage> &getSwapChainImages(void) const noexcept { return this->swapChain->swapChainImages; }
	const std::vector<VkImageView> &getSwapChainImageViews(void) const noexcept {
		return this->swapChain->swapChainImageViews;
	}

	const std::shared_ptr<VKDevice> &getLogicalDevice(void) const noexcept { return this->device; }

	VkPhysicalDevice physicalDevice(void) const;
	void setPhysicalDevice(VkPhysicalDevice device);
	std::vector<VkQueue> getQueues(void) const noexcept;
	const std::vector<VkPhysicalDevice> &availablePhysicalDevices(void) const;

  public:
	virtual void vsync(bool state);

	virtual void setFullScreen(bool fullscreen);

	virtual void swapBuffer(void);

  protected: /*	Internal method for creating swapchains.	*/
	virtual void createSwapChain(void);
	virtual void recreateSwapChain(void);
	virtual void cleanSwapChain(void);
	VkFormat findDepthFormat();

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
		int currentFrame = 0;
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
	SwapchainBuffers *swapChain; // TODO remove as pointer

	/*  */
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	std::vector<VkFence> imageAvailableFence;

	/*	*/
	VkCommandPool cmd_pool;
	VkCommandPool compute_pool;
	VkCommandPool transfer_pool;
};

#endif
