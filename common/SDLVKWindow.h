/**
	Vulkan common and core library
	Copyright (C) 2021  Valdemar Lindberg
*/
#pragma once
#include "FragDef.h"
#include "IWindow.h"
#include "VulkanCore.h"
#include "vulkan/vulkan_core.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <string>

namespace vksample {

	/**
	 * @brief 
	 * 
	 */
	class FVDECLSPEC SDLVKWindow : public IVKWindow {
	  public:
		SDLVKWindow();
		~SDLVKWindow() override;

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

		void setFullScreen(bool fullscreen) override;
		void setFullScreen(fragcore::Display &display) override;

		bool isFullScreen() const override;

		void setBordered(bool borded) override;

		float getGamma() const override;

		void setGamma(float gamma) override;

		void setMinimumSize(int width, int height) override;
		void getMinimumSize(int *width, int *height) override;
		void setMaximumSize(int width, int height) override;
		void getMaximumSize(int *width, int *height) override;

		fragcore::Display *getCurrentDisplay() const override;

		intptr_t getNativePtr() const override; /*  Get native window reference object. */

		VkSurfaceKHR createSurface(const std::shared_ptr<fvkcore::VulkanCore> &instance) override;
		// virtual std::vector<const char*> requiredVulkanExtensions();

	  protected:
		SDL_Window *window;
	};
} // namespace vksample