#define VK_USE_PLATFORM_XLIB_KHR
#include "VKWindow.h"
#include <VKDevice.h>
#include <VKHelper.h>
#include <cassert>
#include <stdexcept>
#include <vulkan/vulkan.h>

VKWindow::~VKWindow() {

	/*	Relase*/
	this->Release();

	cleanSwapChain();

	/*	*/
	for (size_t i = 0; i < this->renderFinishedSemaphores.size(); i++) {
		vkDestroySemaphore(getDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(getDevice(), inFlightFences[i], nullptr);
	}

	/*	*/
	vkDestroyCommandPool(getDevice(), this->cmd_pool, nullptr);

	vkDestroySurfaceKHR(core->getHandle(), this->surface, nullptr);

	this->close();
	delete proxyWindow;
}

VKWindow::VKWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device, int x, int y, int width,
				   int height) {

	this->proxyWindow = new SDLWindow();

	SDL_DisplayMode displaymode;
	SDL_GetCurrentDisplayMode(0, &displaymode);
	if (x == -1 && y == -1) {
		x = displaymode.w / 4;
		y = displaymode.h / 4;
	}

	if (width == -1 && height == -1) {
		width = displaymode.w / 2;
		height = displaymode.h / 2;
	}

	this->setSize(width, height);
	this->setPosition(x, y);

	/*  Create surface. */
	this->surface = this->createSurface(core);

	/*	*/
	this->device = device;
	this->core = core;
	this->swapChain = new SwapchainBuffers();

	/*  Create command pool.    */
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.queueFamilyIndex = device->getDefaultGraphicQueueIndex();
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	/*  Create command pool.    */
	VKS_VALIDATE(vkCreateCommandPool(getDevice(), &cmdPoolCreateInfo, NULL, &this->cmd_pool));

	/*	Create swap chain.	*/
	createSwapChain();

	const int MAX_FRAMES_IN_FLIGHT = this->getSwapChainImageCount();
	this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	/*	*/
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	/*	*/
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VKS_VALIDATE(vkCreateSemaphore(getDevice(), &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]));
		VKS_VALIDATE(vkCreateSemaphore(getDevice(), &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]));
		VKS_VALIDATE(vkCreateFence(getDevice(), &fenceInfo, nullptr, &this->inFlightFences[i]));
	}
}

uint32_t VKWindow::getSwapChainImageCount() const noexcept { return this->swapChain->swapChainImages.size(); }

uint32_t VKWindow::getCurrentFrameIndex() const noexcept { return this->swapChain->currentFrame; }

VkDevice VKWindow::getDevice() const noexcept { return device->getHandle(); }

VkFramebuffer VKWindow::getDefaultFrameBuffer() const noexcept {
	return this->swapChain->swapChainFramebuffers[this->swapChain->currentFrame];
}

VkRenderPass VKWindow::getDefaultRenderPass() const noexcept { return this->swapChain->renderPass; }
VkCommandPool VKWindow::getGraphicCommandPool() const noexcept { return this->cmd_pool; }
VkImage VKWindow::getDefaultImage() const { return this->swapChain->swapChainImages[this->swapChain->currentFrame]; }
VkImageView VKWindow::getDefaultImageView() const {
	return this->swapChain->swapChainImageViews[this->swapChain->currentFrame];
}

VkFormat VKWindow::getDefaultImageFormat() const noexcept { return this->swapChain->swapChainImageFormat; }

VkQueue VKWindow::getDefaultGraphicQueue() const { return this->device->getDefaultGraphicQueue(); }

VkQueue VKWindow::getDefaultComputeQueue() const { return this->device->getDefaultCompute(); }

const std::vector<VkImage> &VKWindow::getSwapChainImages() const noexcept { return this->swapChain->swapChainImages; }
const std::vector<VkImageView> &VKWindow::getSwapChainImageViews() const noexcept {
	return this->swapChain->swapChainImageViews;
}

const std::shared_ptr<VKDevice> &VKWindow::getVKDevice() const noexcept { return this->device; }
const std::shared_ptr<PhysicalDevice> VKWindow::getPhysicalDevice() const noexcept {
	// return this->device->getPhysicalDevice();
}

VkPhysicalDevice VKWindow::physicalDevice() const { return device->getPhysicalDevices()[0]->getHandle(); }

void VKWindow::setPhysicalDevice(VkPhysicalDevice device) {}
std::vector<VkQueue> VKWindow::getQueues() const noexcept {}

const std::vector<VkPhysicalDevice> &VKWindow::availablePhysicalDevices() const { return {}; }

VkCommandBuffer VKWindow::getCurrentCommandBuffer() const noexcept {
	return this->swapChain->commandBuffers[getCurrentFrameIndex()];
}
size_t VKWindow::getNrCommandBuffers() const noexcept { return this->swapChain->commandBuffers.size(); }

VkCommandBuffer VKWindow::getCommandBuffers(unsigned int index) const noexcept {
	return this->swapChain->commandBuffers[index];
}

VkFramebuffer VKWindow::getFrameBuffer(unsigned int index) const noexcept {
	return this->swapChain->swapChainFramebuffers[index];
}

void VKWindow::swapBuffer() {
	VkResult result;

	vkWaitForFences(getDevice(), 1, &this->inFlightFences[this->swapChain->currentFrame], VK_TRUE, UINT64_MAX);

	/*  */
	uint32_t imageIndex;
	result = vkAcquireNextImageKHR(getDevice(), this->swapChain->swapchain, UINT64_MAX,
								   this->imageAvailableSemaphores[this->swapChain->currentFrame], VK_NULL_HANDLE,
								   &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	} else
		VKS_VALIDATE(result);

	if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(getDevice(), 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	/*	*/
	this->imagesInFlight[imageIndex] = this->inFlightFences[this->swapChain->currentFrame];

	/*	*/
	VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->swapChain->currentFrame]};

	vkResetFences(getDevice(), 1, &this->inFlightFences[this->swapChain->currentFrame]);

	this->getVKDevice()->submitCommands(getDefaultGraphicQueue(), {this->swapChain->commandBuffers[imageIndex]},
										{this->imageAvailableSemaphores[this->swapChain->currentFrame]},
										{this->renderFinishedSemaphores[this->swapChain->currentFrame]},
										this->inFlightFences[this->swapChain->currentFrame],
										{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT});

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	/*	*/
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &this->swapChain->swapchain;

	/*	*/
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	/*	*/
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(this->device->getDefaultPresent(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// framebufferResized = false;
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw cxxexcept::RuntimeException("failed to present swap chain image!");
	}

	/*  Compute current frame.  */
	this->swapChain->currentFrame =
		(this->swapChain->currentFrame + 1) % std::min((uint32_t)this->inFlightFences.size(), getSwapChainImageCount());
}

void VKWindow::createSwapChain() {
	const std::shared_ptr<PhysicalDevice> &physicalDevice = device->getPhysicalDevice(0);

	/*  */
	VKHelper::SwapChainSupportDetails swapChainSupport =
		VKHelper::querySwapChainSupport(physicalDevice->getHandle(), this->surface);

	/*	TODO add option support.	*/
	VkSurfaceFormatKHR surfaceFormat = VKHelper::selectSurfaceFormat(swapChainSupport.formats, swapChainSupport.formats,
																	 VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
	// TODO add support to determine which present mode.
	std::vector<VkPresentModeKHR> requestedPresentModes = {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR,
														   VK_PRESENT_MODE_FIFO_KHR};
	const VkPresentModeKHR presentMode =
		VKHelper::chooseSwapPresentMode(swapChainSupport.presentModes, requestedPresentModes);
	const VkExtent2D extent =
		VKHelper::chooseSwapExtent(swapChainSupport.capabilities, {(uint32_t)this->width(), (uint32_t)this->height()});

	/*	Reset frame counter.	*/
	this->swapChain->currentFrame = 0;

	/*	TODO evoluate if this is thec correct.	*/
	/*	Compute number of image to use in the swapchain.	*/
	uint32_t imageCount =
		std::max((uint32_t)swapChainSupport.capabilities.minImageCount, (uint32_t)1); /*	Atleast one.	*/
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		/*	Clamp it to number of semaphores/fences.	*/
		imageCount = std::max(swapChainSupport.capabilities.maxImageCount, (uint32_t)imagesInFlight.size());
	}

	/*	*/
	const VKHelper::QueueFamilyIndices indices =
		VKHelper::findQueueFamilies(physicalDevice->getHandle(), this->surface);
	std::vector<uint32_t> queueFamilyIndices = {indices.graphicsFamily, indices.presentFamily};

	/*  */
	VkSwapchainCreateInfoKHR createSwapChainInfo = {};
	createSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createSwapChainInfo.pNext = nullptr;
	createSwapChainInfo.surface = this->surface;

	/*  Image configurations.	*/
	createSwapChainInfo.minImageCount = imageCount;
	createSwapChainInfo.imageFormat = surfaceFormat.format;
	createSwapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
	createSwapChainInfo.imageExtent = extent;
	createSwapChainInfo.imageArrayLayers = 1;
	createSwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	/*  */
	if (indices.graphicsFamily != indices.presentFamily) {
		createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createSwapChainInfo.queueFamilyIndexCount = queueFamilyIndices.size();
		createSwapChainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	} else {
		createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	/*  */
	createSwapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createSwapChainInfo.presentMode = presentMode;
	createSwapChainInfo.clipped = VK_TRUE;

	/*	*/
	createSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	/*  Create swapchain.   */
	VKS_VALIDATE(vkCreateSwapchainKHR(getDevice(), &createSwapChainInfo, nullptr, &this->swapChain->swapchain));

	/*  Get the image associated with the swap chain.   */
	uint32_t nrChainImageCount = 1;
	VKS_VALIDATE(vkGetSwapchainImagesKHR(getDevice(), this->swapChain->swapchain, &nrChainImageCount, nullptr));

	this->swapChain->swapChainImages.resize(nrChainImageCount);
	VKS_VALIDATE(vkGetSwapchainImagesKHR(getDevice(), this->swapChain->swapchain, &nrChainImageCount,
										 this->swapChain->swapChainImages.data()));

	this->swapChain->swapChainImageFormat = surfaceFormat.format;
	this->swapChain->chainExtend = extent;
	this->imagesInFlight.resize(this->swapChain->swapChainImages.size(), VK_NULL_HANDLE);

	/*	*/
	this->swapChain->swapChainImageViews.resize(this->swapChain->swapChainImages.size());

	for (size_t i = 0; i < this->swapChain->swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = this->swapChain->swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = this->swapChain->swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VKS_VALIDATE(vkCreateImageView(getDevice(), &createInfo, nullptr, &this->swapChain->swapChainImageViews[i]));
	}

	VkFormat depthFormat = findDepthFormat();

	const VkPhysicalDeviceMemoryProperties &memProps = getVKDevice()->getPhysicalDevices()[0]->getMemoryProperties();

	VKHelper::createImage(getDevice(), this->swapChain->chainExtend.width, this->swapChain->chainExtend.height, 1,
						  depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProps, this->swapChain->depthImage,
						  this->swapChain->depthImageMemory);
	this->swapChain->depthImageView = VKHelper::createImageView(
		getDevice(), this->swapChain->depthImage, VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	/*	Renderpass	*/
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = this->swapChain->swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VKS_VALIDATE(vkCreateRenderPass(getDevice(), &renderPassInfo, nullptr, &this->swapChain->renderPass));

	/*	Framebuffer.	*/
	// TODO add support.
	this->swapChain->swapChainFramebuffers.resize(this->swapChain->swapChainImageViews.size());

	for (size_t i = 0; i < this->swapChain->swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {this->swapChain->swapChainImageViews[i],
												  this->swapChain->depthImageView};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = this->swapChain->renderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = this->swapChain->chainExtend.width;
		framebufferInfo.height = this->swapChain->chainExtend.height;
		framebufferInfo.layers = 1;

		VKS_VALIDATE(
			vkCreateFramebuffer(getDevice(), &framebufferInfo, nullptr, &this->swapChain->swapChainFramebuffers[i]));
	}

	/*	Command buffers*/
	this->swapChain->commandBuffers.resize(this->swapChain->swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.commandPool = this->cmd_pool;
	cmdBufAllocInfo.commandBufferCount = this->swapChain->swapChainImages.size();
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VKS_VALIDATE(vkAllocateCommandBuffers(getDevice(), &cmdBufAllocInfo, this->swapChain->commandBuffers.data()));
}

void VKWindow::recreateSwapChain() {

	vkDeviceWaitIdle(getDevice());

	cleanSwapChain();

	createSwapChain();
}

void VKWindow::cleanSwapChain() {
	for (auto framebuffer : swapChain->swapChainFramebuffers) {
		vkDestroyFramebuffer(getDevice(), framebuffer, nullptr);
	}
	swapChain->swapChainFramebuffers.clear();

	/*	*/
	vkFreeCommandBuffers(getDevice(), this->cmd_pool, static_cast<uint32_t>(swapChain->commandBuffers.size()),
						 swapChain->commandBuffers.data());
	swapChain->commandBuffers.clear();

	vkDestroyRenderPass(getDevice(), swapChain->renderPass, nullptr);

	/*	*/
	for (auto imageView : swapChain->swapChainImageViews) {
		vkDestroyImageView(getDevice(), imageView, nullptr);
	}
	swapChain->swapChainImageViews.clear();

	/*	Release depth/stencil.	*/
	vkDestroyImageView(getDevice(), swapChain->depthImageView, nullptr);
	vkDestroyImage(getDevice(), swapChain->depthImage, nullptr);
	vkFreeMemory(getDevice(), swapChain->depthImageMemory, nullptr);

	vkDestroySwapchainKHR(getDevice(), this->swapChain->swapchain, nullptr);
}
VkFormat VKWindow::findDepthFormat() {
	return VKHelper::findSupportedFormat(
		physicalDevice(), {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSurfaceKHR VKWindow::createSurface() {
	VkXlibSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.window = proxyWindow->getNativePtr();
	createInfo.pNext = nullptr;
	// createInfo.dpy = proxyWindow->getNativePtr();
	createInfo.flags = 0;

	VkSurfaceKHR surface;
	VKS_VALIDATE(vkCreateXlibSurfaceKHR(core->getHandle(), &createInfo, nullptr, &surface));

	return surface;
}

void VKWindow::vsync(bool state) {
	if (state != this->swapChain->vsync) {
		this->swapChain->vsync = state;
		recreateSwapChain();
	}
}

void VKWindow::setFullScreen(bool fullscreen) { this->setFullScreen(fullscreen); }

void VKWindow::Initialize() {}

void VKWindow::Release() {}

void VKWindow::draw() {}

void VKWindow::onResize(int width, int height) {}

void VKWindow::run() {
	/*	*/
	this->Initialize();

	SDL_Event event = {};
	bool isAlive = true;
	bool visible = true;

	while (isAlive) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				goto finished;
				// return; /*  Exit.  */
			case SDL_KEYDOWN:
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_CLOSE:
					goto finished;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
					recreateSwapChain();
					onResize(event.window.data1, event.window.data2);
					break;
				case SDL_WINDOWEVENT_HIDDEN:
				case SDL_WINDOWEVENT_MINIMIZED:
					visible = false;
					break;
				case SDL_WINDOWEVENT_EXPOSED:
				case SDL_WINDOWEVENT_SHOWN:
					visible = true;
					break;
				}
				break;

			default:
				break;
			}
		}
		if (visible) {
			this->draw();
			this->swapBuffer();
		}
	}
finished:
	vkDeviceWaitIdle(getDevice());

	/*	Release all the resources associated with the window application.	*/
	this->Release();
}

void VKWindow::show() { proxyWindow->show(); }

void VKWindow::hide() { proxyWindow->hide(); }

void VKWindow::close() { proxyWindow->close(); }

void VKWindow::focus() { proxyWindow->focus(); }

void VKWindow::restore() { proxyWindow->restore(); }

void VKWindow::maximize() { proxyWindow->maximize(); }

void VKWindow::minimize() { proxyWindow->minimize(); }

void VKWindow::setTitle(const std::string &title) { proxyWindow->setTitle(title); }

std::string VKWindow::getTitle() const { return proxyWindow->getTitle(); }

int VKWindow::x() const noexcept { return proxyWindow->x(); }
int VKWindow::y() const noexcept { return proxyWindow->y(); }

int VKWindow::width() const noexcept { return proxyWindow->width(); }
int VKWindow::height() const noexcept { return proxyWindow->height(); }

void VKWindow::getPosition(int *x, int *y) const { proxyWindow->getPosition(x, y); }

void VKWindow::setPosition(int x, int y) noexcept { proxyWindow->setPosition(x, y); }

void VKWindow::setSize(int width, int height) noexcept { proxyWindow->setSize(width, height); }

void VKWindow::getSize(int *width, int *height) const { proxyWindow->getSize(width, height); }

void VKWindow::resizable(bool resizable) noexcept { proxyWindow->resizable(resizable); }

bool VKWindow::isFullScreen() const { return proxyWindow->isFullScreen(); }

void VKWindow::setBordered(bool boarded) { proxyWindow->setBordered(boarded); }

float VKWindow::getGamma() const { return proxyWindow->getGamma(); }

void VKWindow::setGamma(float gamma) { proxyWindow->setGamma(gamma); }

void VKWindow::setMinimumSize(int width, int height) { proxyWindow->setMinimumSize(width, height); }
void VKWindow::getMinimumSize(int *width, int *height) { proxyWindow->getMinimumSize(width, height); }
void VKWindow::setMaximumSize(int width, int height) { proxyWindow->setMaximumSize(width, height); }
void VKWindow::getMaximumSize(int *width, int *height) { proxyWindow->getMaximumSize(width, height); }

intptr_t VKWindow::getNativePtr() const { return proxyWindow->getNativePtr(); }

VkSurfaceKHR VKWindow::createSurface(std::shared_ptr<VulkanCore> &instance) {
	return proxyWindow->createSurface(instance);
}

std::vector<const char *> VKWindow::getRequiredDeviceExtensions() {

	std::vector<const char *> usedInstanceExtensionNames;

	// TODO be replace with own code!
	// SDL_Window *tmpWindow = SDL_CreateWindow("", 0, 0, 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
	// if (tmpWindow == NULL)
	// 	throw cxxexcept::RuntimeException("Failed to create Tmp Vulkan window - {}", SDL_GetError());
	// /*	*/
	// unsigned int count;
	// if (!SDL_Vulkan_GetInstanceExtensions(tmpWindow, &count, nullptr))
	// 	throw cxxexcept::RuntimeException("SDL_Vulkan_GetInstanceExtensions");
	// unsigned int additional_extension_count = (unsigned int)usedInstanceExtensionNames.size();
	// usedInstanceExtensionNames.resize((size_t)(additional_extension_count + count));
	// if (!SDL_Vulkan_GetInstanceExtensions(tmpWindow, &count,
	// &usedInstanceExtensionNames[additional_extension_count])) 	throw
	// cxxexcept::RuntimeException("SDL_Vulkan_GetInstanceExtensions"); SDL_DestroyWindow(tmpWindow);

	return usedInstanceExtensionNames;
}