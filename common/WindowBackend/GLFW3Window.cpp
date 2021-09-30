#include "../GLFWWindow.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void GLFWWindow::show() noexcept {}

void GLFWWindow::hide() noexcept {}

void GLFWWindow::close(void) noexcept {
	this->hide();
	glfwDestroyWindow(window);
}

void GLFWWindow::setPosition(int x, int y) noexcept { glfwSetWindowPos(this->window, x, y); }

void GLFWWindow::setSize(int width, int height) noexcept {}

void GLFWWindow::getPosition(int *x, int *y) const {}

void GLFWWindow::getSize(int *width, int *height) const { glfwGetWindowSize(this->window, width, height); }
void GLFWWindow::setTitle(const std::string &title) {}

std::string GLFWWindow::getTitle(void) {}

int GLFWWindow::x(void) const noexcept {
	int x, y;

	return x;
}
int GLFWWindow::y(void) const noexcept {
	int x, y;

	return y;
}
void GLFWWindow::resizable(bool resizable) noexcept {}

void GLFWWindow::setFullScreen(bool fullscreen) {}

bool GLFWWindow::isFullScreen(void) const { return false; }

void GLFWWindow::setBordered(bool bordered) {}

int GLFWWindow::width(void) const noexcept {
	int w, h;
	getSize(&w, &h);
	return w;
}
int GLFWWindow::height(void) const noexcept {
	int w, h;
	getSize(&w, &h);
	return h;
}

void GLFWWindow::setMinimumSize(int width, int height) {}
void GLFWWindow::getMinimumSize(int *width, int *height) {}

void GLFWWindow::setMaximumSize(int width, int height) {}
void GLFWWindow::getMaximumSize(int *width, int *height) {}

void GLFWWindow::focus(void) {}

void GLFWWindow::restore(void) {}

void GLFWWindow::maximize(void) {}

void GLFWWindow::minimize(void) {}