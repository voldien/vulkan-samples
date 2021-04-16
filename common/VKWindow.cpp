#include "VKWindow.h"
#include "VKDevice.h"
#include "VKHelper.h"
#include "common.hpp"
#include <SDL2/SDL_vulkan.h>
#include <cassert>
#include <stdexcept>
#include <vulkan/vulkan.h>
//#include <fmt/core.h>

VKWindow::~VKWindow(void) {
	vkDeviceWaitIdle(getDevice());

	/*	Relase*/
	this->Release();

	cleanSwapChain();

	/*	*/
	for (size_t i = 0; i < this->imagesInFlight.size(); i++) {
		vkDestroySemaphore(getDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(getDevice(), inFlightFences[i], nullptr);
	}

	/*	*/
	vkDestroyCommandPool(getDevice(), this->cmd_pool, nullptr);

	vkDestroySurfaceKHR(core->getHandle(), this->surface, nullptr);

	this->close();
}

VKWindow::VKWindow(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device, int x, int y, int width,
				   int height) {

	if (SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error(fmt::format("Failed to init subsystem {}", SDL_GetError()));
	}

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

	/*  Create Vulkan window.   */
	this->window = SDL_CreateWindow("Vulkan Sample", x, y, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
	if (window == NULL) {
		// throw std::runtime_error(fvformatf("failed create window - %s", SDL_GetError()));
	}

	/*  Create surface. */
	bool surfaceResult = SDL_Vulkan_CreateSurface(this->window, core->getHandle(), &this->surface);
	if (surfaceResult == SDL_FALSE) {
		throw std::runtime_error("failed create vulkan surface - %s");
	}

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
	VK_CHECK(vkCreateCommandPool(getDevice(), &cmdPoolCreateInfo, NULL, &this->cmd_pool));

	/*	Create swap chain.	*/
	createSwapChain();

	// TODO resolve and move the the chain
	const int MAX_FRAMES_IN_FLIGHT = 3;
	this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	this->imagesInFlight.resize(this->swapChain->swapChainImages.size(),
								VK_NULL_HANDLE); // TODO resolve this->swapChainImages.size()

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	/*	*/
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// TODO improve error message.
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(getDevice(), &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(getDevice(), &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(getDevice(), &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void VKWindow::swapBuffer(void) {
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
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to acquire next image - {}");
	}

	if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(getDevice(), 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	/*	*/
	this->imagesInFlight[imageIndex] = this->inFlightFences[this->swapChain->currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->swapChain->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	/*	*/
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->swapChain->commandBuffers[imageIndex];

	/*	*/
	VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->swapChain->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(getDevice(), 1, &this->inFlightFences[this->swapChain->currentFrame]);

	VK_CHECK(vkQueueSubmit(device->getDefaultGraphicQueue(), 1, &submitInfo,
						   this->inFlightFences[this->swapChain->currentFrame]));

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
		throw std::runtime_error("failed to present swap chain image!");
	}

	/*  Compute current frame.  */
	this->swapChain->currentFrame = (this->swapChain->currentFrame + 1) % this->inFlightFences.size();
}

void VKWindow::createSwapChain(void) {

	/*  */
	VKHelper::SwapChainSupportDetails swapChainSupport =
		VKHelper::querySwapChainSupport(device->getPhysicalDevices()[0]->getHandle(), this->surface);

	/*	*/
	VkSurfaceFormatKHR surfaceFormat = VKHelper::chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = VKHelper::chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = VKHelper::chooseSwapExtent(swapChainSupport.capabilities, {width(), height()});

	/*	*/
	this->swapChain->currentFrame = 0;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VKHelper::QueueFamilyIndices indices =
		VKHelper::findQueueFamilies(device->getPhysicalDevices()[0]->getHandle(), this->surface);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

	/*  */
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = this->surface;

	/*  */
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	/*  */
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	/*  */
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	/*  Create swapchain.   */
	VK_CHECK(vkCreateSwapchainKHR(getDevice(), &createInfo, NULL, &this->swapChain->swapchain));

	/*  Get the image associated with the swap chain.   */
	uint32_t nrChainImageCount = 1;
	VK_CHECK(vkGetSwapchainImagesKHR(getDevice(), this->swapChain->swapchain, &nrChainImageCount, NULL));

	this->swapChain->swapChainImages.resize(nrChainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(getDevice(), this->swapChain->swapchain, &nrChainImageCount,
									 this->swapChain->swapChainImages.data()));

	this->swapChain->swapChainImageFormat = surfaceFormat.format;
	this->swapChain->chainExtend = extent;

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

		VK_CHECK(vkCreateImageView(getDevice(), &createInfo, nullptr, &this->swapChain->swapChainImageViews[i]));
	}

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

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = NULL;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(getDevice(), &renderPassInfo, nullptr, &this->swapChain->renderPass));

	/*	Framebuffer.	*/
	// TODO add support.
	this->swapChain->swapChainFramebuffers.resize(this->swapChain->swapChainImageViews.size());

	for (size_t i = 0; i < this->swapChain->swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {this->swapChain->swapChainImageViews[i]};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = this->swapChain->renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = this->swapChain->chainExtend.width;
		framebufferInfo.height = this->swapChain->chainExtend.height;
		framebufferInfo.layers = 1;

		VK_CHECK(
			vkCreateFramebuffer(getDevice(), &framebufferInfo, nullptr, &this->swapChain->swapChainFramebuffers[i]));
	}

	/*	Command buffers*/
	this->swapChain->commandBuffers.resize(this->swapChain->swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.commandPool = this->cmd_pool;
	cmdBufAllocInfo.commandBufferCount = this->swapChain->swapChainImages.size();
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VK_CHECK(vkAllocateCommandBuffers(getDevice(), &cmdBufAllocInfo, this->swapChain->commandBuffers.data()));
}

void VKWindow::recreateSwapChain(void) {

	vkDeviceWaitIdle(getDevice());

	cleanSwapChain();

	createSwapChain();

	imagesInFlight.resize(this->swapChain->swapChainImages.size(), VK_NULL_HANDLE);
}

void VKWindow::cleanSwapChain(void) {
	for (auto framebuffer : swapChain->swapChainFramebuffers) {
		vkDestroyFramebuffer(getDevice(), framebuffer, nullptr);
	}
	swapChain->swapChainFramebuffers.clear();

	vkFreeCommandBuffers(getDevice(), this->cmd_pool, static_cast<uint32_t>(swapChain->commandBuffers.size()),
						 swapChain->commandBuffers.data());
	swapChain->commandBuffers.clear();

	vkDestroyRenderPass(getDevice(), swapChain->renderPass, nullptr);

	for (auto imageView : swapChain->swapChainImageViews) {
		vkDestroyImageView(getDevice(), imageView, nullptr);
	}
	swapChain->swapChainImageViews.clear();

	vkDestroySwapchainKHR(getDevice(), this->swapChain->swapchain, nullptr);
}

int VKWindow::swapChainImageCount() const { return this->swapChain->swapChainImages.size(); }

int VKWindow::getCurrentFrame(void) const { return this->swapChain->currentFrame; }

VkDevice VKWindow::getDevice(void) const { return device->getHandle(); }

VkFramebuffer VKWindow::getDefaultFrameBuffer(void) const {
	return this->swapChain->swapChainFramebuffers[this->swapChain->currentFrame];
}

VkCommandBuffer VKWindow::getCurrentCommandBuffer(void) const {
	return this->swapChain->commandBuffers[this->swapChain->currentFrame];
}
VkRenderPass VKWindow::getDefaultRenderPass(void) const { return this->swapChain->renderPass; }
VkCommandPool VKWindow::getGraphicCommandPool(void) const { return this->cmd_pool; }
VkImage VKWindow::getDefaultImage(void) const {
	return this->swapChain->swapChainImages[this->swapChain->currentFrame];
}

VkQueue VKWindow::getDefaultGraphicQueue(void) const { return this->device->getDefaultGraphicQueue(); }

VkPhysicalDevice VKWindow::physicalDevice() const {
	return device->getPhysicalDevices()[0]->getHandle();
	// physicalDevices[0];
}

std::vector<VkPhysicalDevice> VKWindow::getPhyiscalDevices(void) {}

std::vector<VkCommandBuffer> &VKWindow::getCommandBuffers(void) const noexcept {
	return this->swapChain->commandBuffers;
}

std::vector<VkFramebuffer> &VKWindow::getFrameBuffers(void) const noexcept {
	return this->swapChain->swapChainFramebuffers;
}

void VKWindow::vsync(bool state) {}

void VKWindow::setFullScreen(bool fullscreen) { SDLWindow::setFullScreen(fullscreen); }

void VKWindow::Initialize(void) {}

void VKWindow::Release(void) {}

void VKWindow::draw(void) {}

void VKWindow::onResize(int width, int height) {}

void VKWindow::run(void) {
	/*	*/
	this->Initialize();

	SDL_Event event = {};
	bool isAlive = true;
	bool visible;

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
					return;
				// case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
					recreateSwapChain();
					onResize(event.window.data1, event.window.data2);
				case SDL_WINDOWEVENT_HIDDEN:
				case SDL_WINDOWEVENT_MINIMIZED:
					visible = 0;
					break;
				case SDL_WINDOWEVENT_EXPOSED:
				case SDL_WINDOWEVENT_SHOWN:
					visible = 1;
					break;
				}
				break;

			default:
				break;
			}
		}
		this->draw();
		this->swapBuffer();
	}
finished:
	vkQueueWaitIdle(getDefaultGraphicQueue());
	this->Release();
}