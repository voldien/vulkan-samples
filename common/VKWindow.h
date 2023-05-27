#pragma once
#include "IWindow.h"
#include "SDLWindow.h"
#include "Util/Time.hpp"
#include "VKSampleBase.h"
#include <iostream>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

// TODO use Window based on either fragcore or MIMI.
using namespace fvkcore;

// TODO rename
class VKWindow : public vkscommon::VKSampleSessionBase, public IWindow {
  protected:
	VKWindow() = default;

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
	virtual ~VKWindow();

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
	virtual void release();
	/**
	 * @brief
	 *
	 */
	virtual void draw();

	/**
	 * @brief
	 *
	 */
	virtual void update();

	/**
	 * @brief
	 *
	 */
	virtual void run() override;
	// virtual void run(VkCommandBuffer cmdBuffer);
	/**
	 * @brief
	 *
	 */
	virtual void onResize(int width, int height);

	// virtual void createLogisticDevice(VkQueueFlags queues);

	void captureScreenShot();

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

	const std::shared_ptr<PhysicalDevice> getPhysicalDevice() const noexcept;

	VkPhysicalDevice physicalDevice() const;
	void setPhysicalDevice(VkPhysicalDevice device);
	std::vector<VkQueue> getQueues() const noexcept;
	const std::vector<VkPhysicalDevice> &availablePhysicalDevices() const;

  public:
	virtual void swapBuffer();

  public:
	static std::vector<const char *> getRequiredDeviceExtensions();

  protected: /*	Internal method for creating swapchains.	*/
	virtual void createSwapChain();
	virtual void recreateSwapChain();
	virtual void cleanSwapChain();
	VkFormat findDepthFormat();
	VkSurfaceKHR createSurface();

  public:
	virtual void show() override;

	virtual void hide() override;

	virtual void close() override;

	virtual void focus() override;

	virtual void restore() override;

	virtual void maximize() override;

	virtual void minimize() override;

	virtual void setTitle(const std::string &title) override;

	virtual std::string getTitle() const override;

	virtual int x() const noexcept override;
	virtual int y() const noexcept override;

	virtual int width() const noexcept override;
	virtual int height() const noexcept override;

	virtual void getPosition(int *x, int *y) const override;

	virtual void setPosition(int x, int y) noexcept override;

	virtual void setSize(int width, int height) noexcept override;

	virtual void getSize(int *width, int *height) const override;

	virtual void resizable(bool resizable) noexcept;

	virtual void vsync(bool state);

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen() const override;

	virtual void setBordered(bool boarded);

	virtual float getGamma() const override;

	virtual void setGamma(float gamma) override;

	virtual void setMinimumSize(int width, int height) override;
	virtual void getMinimumSize(int *width, int *height) override;
	virtual void setMaximumSize(int width, int height) override;
	virtual void getMaximumSize(int *width, int *height) override;

	virtual intptr_t getNativePtr() const override; /*  Get native window reference object. */
	virtual VkSurfaceKHR createSurface(const std::shared_ptr<VulkanCore> &instance) override;

  public:
	vkscommon::Time &getTimer() noexcept { return this->time; }

  private:
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

	VkSurfaceKHR surface;
	/*  Collection of swap chain variables. */
	SwapchainBuffers *swapChain; // TODO remove as pointer

	/*  Synchronization.	*/
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	std::vector<VkFence> imageAvailableFence;

	IWindow *proxyWindow;

	vkscommon::Time time;
};
