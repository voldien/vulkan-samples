#include "../SDLVKWindow.h"
#include "VulkanCore.h"
#include <SDL2/SDL_vulkan.h>

using namespace vksample;

SDLVKWindow::SDLVKWindow() {

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
		throw cxxexcept::RuntimeException("Failed to init subsystem {}", SDL_GetError());
	}

	int width = 800;
	int height = 600;

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
													 SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_INPUT_FOCUS);
	this->window =
		SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);

	if (window == nullptr) {
		throw cxxexcept::RuntimeException("failed create window - {}", SDL_GetError());
	}
}
SDLVKWindow::~SDLVKWindow() {
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
}

void SDLVKWindow::show() { SDL_ShowWindow(this->window); }

void SDLVKWindow::hide() { SDL_HideWindow(this->window); }

void SDLVKWindow::close() {
	this->hide();
	SDL_DestroyWindow(this->window);
}

void SDLVKWindow::setTitle(const std::string &title) { SDL_SetWindowTitle(window, title.c_str()); }

std::string SDLVKWindow::getTitle() const { return SDL_GetWindowTitle(window); }

void SDLVKWindow::setPosition(int x, int y) noexcept { SDL_SetWindowPosition(this->window, x, y); }

void SDLVKWindow::setSize(int width, int height) noexcept { SDL_SetWindowSize(this->window, width, height); }

void SDLVKWindow::getPosition(int *x, int *y) const { SDL_GetWindowPosition(this->window, x, y); }

void SDLVKWindow::getSize(int *width, int *height) const { SDL_GetWindowSize(this->window, width, height); }

int SDLVKWindow::x() const noexcept {
	int x = 0, y = 0;
	SDL_GetWindowPosition(this->window, &x, &y);
	return x;
}

int SDLVKWindow::y() const noexcept {
	int x = 0, y = 0;
	SDL_GetWindowPosition(this->window, &x, &y);
	return y;
}
void SDLVKWindow::resizable(bool resizable) noexcept { SDL_SetWindowResizable(this->window, (SDL_bool)resizable); }

void SDLVKWindow::setFullScreen(bool fullscreen) {

	if (fullscreen) {
		SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else {
		SDL_SetWindowFullscreen(this->window, 0);
	}
}
void SDLVKWindow::setFullScreen(fragcore::Display &display) {}

bool SDLVKWindow::isFullScreen() const { return false; }

void SDLVKWindow::setBordered(bool bordered) { SDL_SetWindowBordered(this->window, (SDL_bool)bordered); }

int SDLVKWindow::width() const noexcept {
	int w = 0, h = 0;
	getSize(&w, &h);
	return w;
}
int SDLVKWindow::height() const noexcept {
	int w = 0, h = 0;
	getSize(&w, &h);
	return h;
}

float SDLVKWindow::getGamma() const { return 1.0f; }

void SDLVKWindow::setGamma(float gamma) {
	// TODO set
}

void SDLVKWindow::setMinimumSize(int width, int height) { SDL_SetWindowMinimumSize(this->window, width, height); }
void SDLVKWindow::getMinimumSize(int *width, int *height) { SDL_GetWindowMinimumSize(this->window, width, height); }

void SDLVKWindow::setMaximumSize(int width, int height) { SDL_SetWindowMaximumSize(this->window, width, height); }
void SDLVKWindow::getMaximumSize(int *width, int *height) { SDL_GetWindowMaximumSize(this->window, width, height); }

void SDLVKWindow::focus() { SDL_SetWindowInputFocus(this->window); }

void SDLVKWindow::restore() { SDL_RestoreWindow(this->window); }

void SDLVKWindow::maximize() { SDL_MaximizeWindow(this->window); }

void SDLVKWindow::minimize() { SDL_MinimizeWindow(this->window); }

VkSurfaceKHR SDLVKWindow::createSurface(const std::shared_ptr<fvkcore::VulkanCore> &instance) {
	VkSurfaceKHR surface = nullptr;
	bool surfaceResult = SDL_Vulkan_CreateSurface(this->window, instance->getHandle(), &surface);
	if (surfaceResult == SDL_FALSE) {
		throw cxxexcept::RuntimeException("failed create vulkan surface - {}", SDL_GetError());
	}
	return surface;
}

fragcore::Display *SDLVKWindow::getCurrentDisplay() const {}

intptr_t SDLVKWindow::getNativePtr() const {
	return (intptr_t)this->window;
	// 	SDL_SysWMinfo info;

	// 	SDL_VERSION(&info.version); /* initialize info structure with SDL version info */

	// 	if (SDL_GetWindowWMInfo(window, &info)) { /* the call returns true on success */
	// 		/* success */
	// 		switch (info.subsystem) {
	// 		case SDL_SYSWM_UNKNOWN:
	// 			return 0;
	// 		case SDL_SYSWM_WINDOWS:
	// #if defined(SDL_VIDEO_DRIVER_WINDOWS)
	// 			return info.info.win.window;
	// #endif
	// 		case SDL_SYSWM_X11:
	// #if defined(SDL_VIDEO_DRIVER_X11)
	// 			return (intptr_t)info.info.x11.window;
	// #endif
	// #if SDL_VERSION_ATLEAST(2, 0, 3)
	// 		case SDL_SYSWM_WINRT:
	// #endif
	// 		case SDL_SYSWM_DIRECTFB:
	// 		case SDL_SYSWM_COCOA:
	// 		case SDL_SYSWM_UIKIT:
	// #if SDL_VERSION_ATLEAST(2, 0, 2)
	// 		case SDL_SYSWM_WAYLAND:
	// #if defined(SDL_VIDEO_DRIVER_WAYLAND)
	// 			return (intptr_t)info.info.wl.surface;
	// #endif
	// 		case SDL_SYSWM_MIR:
	// #if defined(SDL_VIDEO_DRIVER_MIR)
	// 			return (intptr_t)info.info.mir.surface;
	// #endif
	// #endif
	// #if SDL_VERSION_ATLEAST(2, 0, 4)
	// 		case SDL_SYSWM_ANDROID:
	// #endif
	// #if SDL_VERSION_ATLEAST(2, 0, 5)
	// 		case SDL_SYSWM_VIVANTE:
	// 			break;
	// #endif
	// 		default:
	// 			break;
	// 		}
	// 	} // else
	// 	  //     throw RuntimeException(fmt::format("%s", SDL_GetError()));
	// 	  // throw NotImplementedException("Window format not implemented");
}