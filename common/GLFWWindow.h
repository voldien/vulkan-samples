
#ifndef _VKCOMMON_GLFW_WINDOW_H_
#define _VKCOMMON_GLFW_WINDOW_H_ 1
#include "IWindow.h"
#include <GLFW/glfw3.h>
#include <string>

/**
 * @brief
 *
 */
class GLFWWindow : public IWindow {
  public:
	virtual void show() noexcept;

	virtual void hide() noexcept;

	virtual void close() noexcept;

	virtual void focus();

	virtual void restore();

	virtual void maximize();

	virtual void minimize();

	virtual void setTitle(const std::string &title);

	virtual std::string getTitle();

	virtual int x() const noexcept;
	virtual int y() const noexcept;

	virtual int width() const noexcept;
	virtual int height() const noexcept;

	virtual void getPosition(int *x, int *y) const;

	virtual void setPosition(int x, int y) noexcept;

	virtual void setSize(int width, int height) noexcept;

	virtual void getSize(int *width, int *height) const;

	virtual void resizable(bool resizable) noexcept;

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen() const;

	virtual void setBordered(bool borded);

	virtual void setMinimumSize(int width, int height);
	virtual void getMinimumSize(int *width, int *height);
	virtual void setMaximumSize(int width, int height);
	virtual void getMaximumSize(int *width, int *height);

  protected:
	GLFWwindow *window;
};

#endif
