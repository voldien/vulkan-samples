#include "SDLWindow.h"

void SDLWindow::show() noexcept { SDL_ShowWindow(this->window); }

void SDLWindow::hide() noexcept { SDL_HideWindow(this->window); }

void SDLWindow::close(void) noexcept {
	this->hide();
	SDL_DestroyWindow(this->window);
}

void SDLWindow::setPosition(int x, int y) noexcept { SDL_SetWindowPosition(this->window, x, y); }

void SDLWindow::setSize(int width, int height) noexcept { SDL_SetWindowSize(this->window, width, height); }

void SDLWindow::getPosition(int *x, int *y) const { SDL_GetWindowPosition(this->window, x, y); }

void SDLWindow::getSize(int *width, int *height) const { SDL_GetWindowSize(this->window, width, height); }
void SDLWindow::setTitle(const std::string &title) { SDL_SetWindowTitle(window, title.c_str()); }

std::string SDLWindow::getTitle(void) { return SDL_GetWindowTitle(window); }

int SDLWindow::x(void) const noexcept {
	int x, y;
	SDL_GetWindowPosition(this->window, &x, &y);
	return x;
}
int SDLWindow::y(void) const noexcept {
	int x, y;
	SDL_GetWindowPosition(this->window, &x, &y);
	return y;
}
void SDLWindow::resizable(bool resizable) noexcept { SDL_SetWindowResizable(this->window, (SDL_bool)resizable); }

void SDLWindow::setFullScreen(bool fullscreen) {

	if (fullscreen)
		SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		SDL_SetWindowFullscreen(this->window, 0);
}

bool SDLWindow::isFullScreen(void) const { return false; }

void SDLWindow::setBordered(bool bordered) { SDL_SetWindowBordered(this->window, (SDL_bool)bordered); }

int SDLWindow::width(void) const noexcept {
	int w, h;
	getSize(&w, &h);
	return w;
}
int SDLWindow::height(void) const noexcept {
	int w, h;
	getSize(&w, &h);
	return h;
}

void SDLWindow::setMinimumSize(int width, int height) { SDL_SetWindowMinimumSize(this->window, width, height); }
void SDLWindow::getMinimumSize(int *width, int *height) { SDL_GetWindowMinimumSize(this->window, width, height); }

void SDLWindow::setMaximumSize(int width, int height) { SDL_SetWindowMaximumSize(this->window, width, height); }
void SDLWindow::getMaximumSize(int *width, int *height) { SDL_GetWindowMaximumSize(this->window, width, height); }

void SDLWindow::focus(void) { SDL_SetWindowInputFocus(this->window); }

void SDLWindow::restore(void) { SDL_RestoreWindow(this->window); }

void SDLWindow::maximize(void) { SDL_MaximizeWindow(this->window); }

void SDLWindow::minimize(void) { SDL_MinimizeWindow(this->window); }