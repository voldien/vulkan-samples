#ifndef _STARTUP_WINDOW_SAMPLE_H_
#define _STARTUP_WINDOW_SAMPLE_H_ 1
#include "Core/VKDevice.h"
#include "Core/VulkanCore.h"
#include "IWindow.h"
#include "SDLWindow.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class VKWindow : public IWindow {
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
	~VKWindow();

  public:
	/**
	 * @brief
	 *
	 */
	virtual void Initialize();
	/**
	 * @brief
	 *
	 */
	virtual void Release();
	/**
	 * @brief
	 *
	 */
	virtual void draw();
	/**
	 * @brief
	 *
	 */
	virtual void run();
	// virtual void run(VkCommandBuffer cmdBuffer);
	/**
	 * @brief
	 *
	 */
	virtual void onResize(int width, int height);

	// virtual void createLogisticDevice(VkQueueFlags queues);

  public: /*	Vulkan methods.	*/
	/*	*/
	VkDevice getDevice() const noexcept;
	/*	*/
	uint32_t getCurrentFrameIndex() const noexcept;
	uint32_t getSwapChainImageCount() const noexcept;
	VkFramebuffer getDefaultFrameBuffer() const noexcept;
	VkRenderPass getDefaultRenderPass() const noexcept;

	VkFramebuffer getFrameBuffer(unsigned int index) const noexcept;
	/*	*/
	VkFormat depthStencilFormat() const noexcept;
	VkImage depthStencilImage() const noexcept;
	VkImageView depthStencilImageView() const noexcept;

	/*	*/
	VkImage getDefaultMSSAColorImage() const noexcept;
	VkImageView getDefaultMSSAColorImageView() const noexcept;

	/*	*/
	VkImage getDefaultImage() const;
	VkImageView getDefaultImageView() const;
	VkFormat getDefaultImageFormat() const noexcept;

	/*	*/
	VkCommandBuffer getCurrentCommandBuffer() const noexcept;
	size_t getNrCommandBuffers() const noexcept;
	VkCommandBuffer getCommandBuffers(unsigned int index) const noexcept;
	VkCommandPool getGraphicCommandPool() const noexcept;

  public:
	// VkCommandPool getComputeCommandPool() const noexcept;
	const VkPhysicalDeviceProperties &physicalDeviceProperties() const noexcept;

	/*	*/
	uint32_t getGraphicQueueIndex() const;
	VkQueue getDefaultGraphicQueue() const;
	VkQueue getDefaultComputeQueue() const;

	const std::vector<VkImage> &getSwapChainImages() const noexcept;
	const std::vector<VkImageView> &getSwapChainImageViews() const noexcept;

	const std::shared_ptr<VKDevice> &getVKDevice() const noexcept;
	const std::shared_ptr<PhysicalDevice> getPhysicalDevice() const noexcept;

	VkPhysicalDevice physicalDevice() const;
	void setPhysicalDevice(VkPhysicalDevice device);
	std::vector<VkQueue> getQueues() const noexcept;
	const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const;

  public:
	virtual void vsync(bool state);

	virtual void setFullScreen(bool fullscreen);

	virtual void swapBuffer();

  public:
	std::vector<const char *> getRequiredExtensions();

  protected: /*	Internal method for creating swapchains.	*/
	virtual void createSwapChain();
	virtual void recreateSwapChain();
	virtual void cleanSwapChain();
	VkFormat findDepthFormat();
	VkSurfaceKHR createSurface();

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
	VkQueue queue; // TODO rename graphicsQueue
	VkQueue presentQueue;

	/*  */
	uint32_t graphics_queue_node_index;
	VkSurfaceKHR surface;
	/*  Collection of swap chain variables. */
	SwapchainBuffers *swapChain; // TODO remove as pointer

	/*  Synchronization.	*/
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	std::vector<VkFence> imageAvailableFence;

	/*	*/
	VkCommandPool cmd_pool;
	VkCommandPool compute_pool;
	VkCommandPool transfer_pool;
	SDL_Window *window;
	IWindow *_window;
};

#endif
