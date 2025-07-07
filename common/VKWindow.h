#pragma once
#include "IWindow.h"
#include "SDLInput.h"
#include "VKSampleBase.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace vksample {

	using namespace fvkcore;

	// TODO rename
	/**
	 * @brief
	 *
	 */
	class VKWindow : public vksample::VKSampleSessionBase, public IVKWindow {
	  protected:
		VKWindow() = delete;

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
		VKWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device, int x, int y, int width,
				 int height);
		VKWindow(const VKWindow &other) = delete;
		~VKWindow() override;

	  public:
		/**
		 * @brief
		 *
		 */
		void Initialize() override;
		/**
		 * @brief
		 *
		 */
		void release() override;
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
		void run() override;
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

	  public:
		// VkCommandPool getComputeCommandPool() const noexcept;
		const VkPhysicalDeviceProperties &physicalDeviceProperties() const noexcept;

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
		void createQueueAndCommandPool();
		virtual void createSwapChain();
		virtual void recreateSwapChain();
		virtual void cleanSwapChain();
		VkFormat findDepthFormat();
		VkSurfaceKHR createSurface();

	  public:
		void show() override;

		void hide() override;

		void close() override;

		void focus() override;

		void restore() override;

		void maximize() override;

		void minimize() override;

		void setTitle(const std::string &title) override;

		std::string getTitle() const override;

		int x() const noexcept override;
		int y() const noexcept override;

		int width() const noexcept override;
		int height() const noexcept override;

		void getPosition(int *x, int *y) const override;

		void setPosition(int x, int y) noexcept override;

		void setSize(int width, int height) noexcept override;

		void getSize(int *width, int *height) const override;

		void resizable(bool resizable) noexcept override;


		virtual void vsync(bool state);

		void setFullScreen(bool fullscreen) override;
		void setFullScreen(fragcore::Display &display) override {}

		bool isFullScreen() const override;

		void setBordered(bool boarded) override;

		float getGamma() const override;

		void setGamma(float gamma) override;

		void setMinimumSize(int width, int height) override;
		void getMinimumSize(int *width, int *height) override;
		void setMaximumSize(int width, int height) override;
		void getMaximumSize(int *width, int *height) override;

		fragcore::Display *getCurrentDisplay() const override { return nullptr; }

		intptr_t getNativePtr() const override; /*  Get native window reference object. */
		VkSurfaceKHR createSurface(const std::shared_ptr<VulkanCore> &instance) override;

		fragcore::Input &getInput() noexcept { return this->input; }
		const fragcore::Input &getInput() const noexcept { return this->input; }

		FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }
		const FPSCounter<float> &getFPSCounter() const noexcept { return this->fpsCounter; }

	  private:
		using SwapchainBuffers = struct SwapchainBuffers {
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
		};

		VkSurfaceKHR surface;
		/*  Collection of swap chain variables. */
		SwapchainBuffers *swapChain; // TODO remove as pointer
		VkQueue presentQueue;
		/*  Synchronization.	*/
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		std::vector<VkFence> imageAvailableFence;
		fragcore::SDLInput input;
		IVKWindow *proxyWindow;
		FPSCounter<float> fpsCounter;
	};

} // namespace vksample